#include "network.h"

#include <PubSubClient.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "config.h"
#include "state.h"

static WiFiClient wifi_client;
static PubSubClient mqtt_client(wifi_client);

String mac2String(uint8_t ar[]) {
  String s;
  for (uint8_t i = 0; i < 6; ++i) {
    char buf[3];
    sprintf(buf, "%02X", ar[i]);  // J-M-L: slight modification, added the 0 in
                                  // the format for padding
    s += buf;
    if (i < 5) s += ':';
  }
  return s;
}

void NetworkSMStates::IdleState::executeOnce() {
  if (m_stateMachine.cmd_state == Network::CMDState::CONNECT) {
    if (m_stateMachine.m_device_state.persisted().wifi_quick_connect) {
      transitionTo<QuickConnectState>();
    } else {
      transitionTo<NormalConnectState>();
    }
  }
}

void NetworkSMStates::QuickConnectState::entry() {
  log_i("[NET] connecting Wi-Fi (quick mode)...");
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);
  m_start_time = millis();
  WiFi.begin();
}

void NetworkSMStates::QuickConnectState::executeOnce() {
  if (m_stateMachine.cmd_state == Network::CMDState::DISCONNECT) {
    transitionTo<DisconnectState>();
  } else if (WiFi.status() == WL_CONNECTED) {
    transitionTo<WifiConnectedState>();
  } else if (millis() - m_start_time > QUICK_WIFI_TIMEOUT) {
    // try again with normal mode
    log_i(
        "[NET] Wi-Fi connect failed (quick mode). Retrying with normal "
        "mode...");
    m_stateMachine.m_device_state.persisted().wifi_quick_connect = false;
    return transitionTo<DisconnectState>();
  }
}

void NetworkSMStates::NormalConnectState::entry() {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);

  // WiFi.begin();
  delay(500);
  // get ssid from esp32 saved config
  wifi_config_t conf;
  if (esp_wifi_get_config(WIFI_IF_STA, &conf)) {
    log_e("failed to get esp wifi config");
  }
  String ssid = reinterpret_cast<const char *>(conf.sta.ssid);
  String psk = reinterpret_cast<const char *>(conf.sta.password);
  log_i("[NET] connecting Wi-Fi (normal mode): SSID: %s", ssid.c_str());
  WiFi.begin(ssid.c_str(), psk.c_str());

  m_start_time = millis();
  m_await_confirm_quick_wifi_settings = false;
}

void NetworkSMStates::NormalConnectState::executeOnce() {
  if (m_stateMachine.cmd_state == Network::CMDState::DISCONNECT) {
    return transitionTo<DisconnectState>();
  } else if (WiFi.status() == WL_CONNECTED) {
    if (m_await_confirm_quick_wifi_settings) {
      m_stateMachine.m_device_state.persisted().wifi_quick_connect = true;
      m_stateMachine.m_device_state.save_persisted();
      m_stateMachine.state = Network::State::W_CONNECTED;
      m_stateMachine.m_device_state.set_ip(WiFi.localIP().toString());
      log_i("[NET] Wi-Fi connected, quick mode settings saved.");
      return transitionTo<MQTTConnectState>();
    } else {
      // get bssid and ch, and save it directly to ESP
      log_i(
          "[NET] Wi-Fi connected (normal mode). Saving settings for "
          "quick mode...");

      String ssid = WiFi.SSID();
      String psk = WiFi.psk();
      uint8_t *bssid = WiFi.BSSID();
      int32_t ch = WiFi.channel();
      log_i("[NET] SSID: %s, BSSID: %s, CH: %d", ssid.c_str(),
            mac2String(bssid).c_str(), ch);

      // WiFi.disconnect(); not required, already done in WiFi.begin()
      WiFi.begin(ssid.c_str(), psk.c_str(), ch, bssid, true);
      m_start_time = millis();
      m_await_confirm_quick_wifi_settings = true;
    }
  } else if (millis() - m_start_time >= WIFI_TIMEOUT) {
    log_w("[NET] Wi-Fi connect failed (normal mode). Retrying...");
    return transitionTo<DisconnectState>();
  }
}

void NetworkSMStates::MQTTConnectState::entry() {
  mqtt_client.setServer(
      m_stateMachine.m_device_state.network().mqtt.server.c_str(),
      m_stateMachine.m_device_state.network().mqtt.port);
  mqtt_client.setBufferSize(MQTT_BUFFER_SIZE);
  mqtt_client.setCallback(
      std::bind(&Network::_mqtt_callback, m_stateMachine, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3));
  // proceed with MQTT connection
  log_i("[NET] connecting MQTT....");
  m_stateMachine._connect_mqtt();
  m_start_time = millis();
  // await
}

void NetworkSMStates::MQTTConnectState::executeOnce() {
  if (m_stateMachine.cmd_state == Network::CMDState::DISCONNECT) {
    return transitionTo<DisconnectState>();
  } else if (mqtt_client.connected()) {
    log_i("[NET] MQTT connected.");
    return transitionTo<FullyConnectedState>();
  } else if (millis() - m_start_time > MQTT_TIMEOUT) {
    if (WiFi.status() == WL_CONNECTED) {
      m_stateMachine.state = Network::State::W_CONNECTED;
      log_w("[NET] MQTT connect failed. Retrying...");
      m_stateMachine._connect_mqtt();
      m_start_time = millis();
    } else {
      log_w(
          "[NET] MQTT connect failed. Wi-Fi not connected. Retrying "
          "Wi-Fi...");
      return transitionTo<DisconnectState>();
    }
  }
}

void NetworkSMStates::WifiConnectedState::executeOnce() {
  m_stateMachine.state = Network::State::W_CONNECTED;
  m_stateMachine.m_device_state.set_ip(WiFi.localIP().toString());
  log_i("[NET] Wi-Fi connected.");
  String ssid = WiFi.SSID();
  m_stateMachine.m_device_state.save_all();
  uint8_t *bssid = WiFi.BSSID();
  int32_t ch = WiFi.channel();
  log_i("[NET] SSID: %s, BSSID: %s, CH: %d", ssid.c_str(),
        mac2String(bssid).c_str(), ch);
  transitionTo<MQTTConnectState>();
}

void NetworkSMStates::DisconnectState::entry() {
  log_i("[NET] disconnecting...");
  mqtt_client.disconnect();
  wifi_client.flush();
  WiFi.disconnect(true, m_stateMachine.erase);
  WiFi.mode(WIFI_OFF);
  m_stateMachine.state = Network::State::DISCONNECTED;
  log_i("[NET] disconnected.");
}

void NetworkSMStates::DisconnectState::executeOnce() {
  return transitionTo<IdleState>();
}

void NetworkSMStates::FullyConnectedState::entry() {
  m_last_conn_check_time = millis();
  if (m_stateMachine.on_connect) {
    m_stateMachine.on_connect();
  }
  m_stateMachine.state = Network::State::M_CONNECTED;
}

void NetworkSMStates::FullyConnectedState::executeOnce() {
  if (m_stateMachine.cmd_state == Network::CMDState::DISCONNECT) {
    return transitionTo<DisconnectState>();
  } else if (millis() - m_last_conn_check_time > NET_CONN_CHECK_INTERVAL) {
    if (WiFi.status() != WL_CONNECTED) {
      log_w("[NET] Wi-Fi connection interrupted. Reconnecting...");
      return transitionTo<DisconnectState>();
    } else if (!mqtt_client.connected()) {
      m_stateMachine.state = Network::State::W_CONNECTED;
      log_w("[NET] MQTT connection interrupted. Reconnecting...");
      return transitionTo<MQTTConnectState>();
    }
    m_last_conn_check_time = millis();
  }
}

void Network::connect() {
  cmd_state = CMDState::CONNECT;
  this->erase = false;
  log_d("[NET] cmd connect");
}

void Network::disconnect(bool erase) {
  cmd_state = CMDState::DISCONNECT;
  this->erase = erase;
  log_d("[NET] cmd disconnect");
}

void Network::update() {
  mqtt_client.loop();
  executeOnce();
}

Network::State Network::get_state() { return state; }

bool Network::publish(const char *topic, const char *payload, bool retained) {
  bool ret;
  if (retained) {
    ret = mqtt_client.publish(topic, payload, true);
  } else {
    ret = mqtt_client.publish(topic, payload);
    delay(10);
  }
  if (ret) {
    log_d("[NET] pub to: %s SUCCESS.", topic);
  } else {
    log_w("[NET] pub to: %s FAIL.", topic);
  }
  return ret;
}

bool Network::subscribe(const String &topic) {
  if (topic.length() <= 0) {
    log_w("[NET] sub to empty topic blocked", topic.c_str());
    return false;
  }
  bool ret;
  ret = mqtt_client.subscribe(topic.c_str());
  delay(10);
  if (ret) {
    log_d("[NET] sub to: %s SUCCESS.", topic.c_str());
  } else {
    log_w("[NET] sub to: %s FAIL.", topic.c_str());
  }
  return ret;
}

void Network::set_mqtt_callback(
    std::function<void(const char *, const char *)> callback) {
  usr_callback = callback;
}

void Network::set_on_connect(std::function<void()> on_connect) {
  this->on_connect = on_connect;
}

bool Network::_connect_mqtt() {
  if (m_device_state.network().mqtt.user.length() > 0 &&
      m_device_state.network().mqtt.password.length() > 0) {
    return mqtt_client.connect(m_device_state.factory().unique_id.c_str(),
                               m_device_state.network().mqtt.user.c_str(),
                               m_device_state.network().mqtt.password.c_str());
  } else {
    return mqtt_client.connect(m_device_state.factory().unique_id.c_str());
  }
}

void Network::_mqtt_callback(const char *topic, uint8_t *payload,
                             uint32_t length) {
  char buff[length + 1];
  memcpy(buff, payload, (size_t)length);
  buff[length] = '\0';  // required so it can be converted to String
  log_d("[NET] msg on topic: %s, len: %d, payload: %s", topic, length, buff);
  if (length < 1) {
    return;
  }
  if (usr_callback != NULL) {
    usr_callback(topic, buff);
  }
}

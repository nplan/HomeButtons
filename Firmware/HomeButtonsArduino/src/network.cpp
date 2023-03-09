#include "network.h"
#include <esp_wifi.h>
#include "config.h"
#include "state.h"

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
  if (sm().command_ == Network::Command::CONNECT) {
    if (sm().device_state_.persisted().wifi_quick_connect) {
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
  if (sm().command_ == Network::Command::DISCONNECT) {
    transitionTo<DisconnectState>();
  } else if (WiFi.status() == WL_CONNECTED) {
    transitionTo<WifiConnectedState>();
  } else if (millis() - m_start_time > QUICK_WIFI_TIMEOUT) {
    // try again with normal mode
    log_i(
        "[NET] Wi-Fi connect failed (quick mode). Retrying with normal "
        "mode...");
    sm().device_state_.persisted().wifi_quick_connect = false;
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
  if (sm().command_ == Network::Command::DISCONNECT) {
    return transitionTo<DisconnectState>();
  } else if (WiFi.status() == WL_CONNECTED) {
    if (m_await_confirm_quick_wifi_settings) {
      log_i("[NET] Wi-Fi connected, quick mode settings saved.");
      sm().device_state_.persisted().wifi_quick_connect = true;
      sm().device_state_.save_persisted();
      return transitionTo<WifiConnectedState>();
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
  sm().mqtt_client_.setServer(
      sm().device_state_.userPreferences().mqtt.server.c_str(),
      sm().device_state_.userPreferences().mqtt.port);
  sm().mqtt_client_.setBufferSize(MQTT_BUFFER_SIZE);
  sm().mqtt_client_.setCallback(
      std::bind(&Network::_mqtt_callback, &sm(), std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3));
  // proceed with MQTT connection
  log_i("[NET] connecting MQTT....");
  sm()._connect_mqtt();
  m_start_time = millis();
  // await
}

void NetworkSMStates::MQTTConnectState::executeOnce() {
  if (sm().command_ == Network::Command::DISCONNECT) {
    return transitionTo<DisconnectState>();
  } else if (sm().mqtt_client_.connected()) {
    log_i("[NET] MQTT connected.");
    return transitionTo<FullyConnectedState>();
  } else if (millis() - m_start_time > MQTT_TIMEOUT) {
    if (WiFi.status() == WL_CONNECTED) {
      sm().state_ = Network::State::W_CONNECTED;
      log_w("[NET] MQTT connect failed. Retrying...");
      sm()._connect_mqtt();
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
  sm().state_ = Network::State::W_CONNECTED;
  sm().device_state_.set_ip(WiFi.localIP());
  log_i("[NET] Wi-Fi connected.");
  String ssid = WiFi.SSID();
  sm().device_state_.save_all();
  uint8_t *bssid = WiFi.BSSID();
  int32_t ch = WiFi.channel();
  log_i("[NET] SSID: %s, BSSID: %s, CH: %d", ssid.c_str(),
        mac2String(bssid).c_str(), ch);
  transitionTo<MQTTConnectState>();
}

void NetworkSMStates::DisconnectState::entry() {
  log_i("[NET] disconnecting...");
  sm().mqtt_client_.disconnect();
  sm().wifi_client_.flush();
  WiFi.disconnect(true, sm().erase_);
  WiFi.mode(WIFI_OFF);
  sm().state_ = Network::State::DISCONNECTED;
  log_i("[NET] disconnected.");
}

void NetworkSMStates::DisconnectState::executeOnce() {
  return transitionTo<IdleState>();
}

void NetworkSMStates::FullyConnectedState::entry() {
  m_last_conn_check_time = millis();
  if (sm().on_connect_callback_) {
    sm().on_connect_callback_();
  }
  sm().state_ = Network::State::M_CONNECTED;
}

void NetworkSMStates::FullyConnectedState::executeOnce() {
  if (sm().command_ == Network::Command::DISCONNECT) {
    return transitionTo<DisconnectState>();
  } else if (millis() - m_last_conn_check_time > NET_CONN_CHECK_INTERVAL) {
    if (WiFi.status() != WL_CONNECTED) {
      log_w("[NET] Wi-Fi connection interrupted. Reconnecting...");
      return transitionTo<DisconnectState>();
    } else if (!sm().mqtt_client_.connected()) {
      sm().state_ = Network::State::W_CONNECTED;
      log_w("[NET] MQTT connection interrupted. Reconnecting...");
      return transitionTo<MQTTConnectState>();
    }
    m_last_conn_check_time = millis();
  } else {
    SM::PublishQueueElement element;
    while (sm().mqtt_publish_queue_ != nullptr &&
           xQueueReceive(sm().mqtt_publish_queue_, &element, 0)) {
      log_d("received payload (topic: %s)", element.topic.c_str());
      sm()._publish_unsafe(element.topic, element.payload.c_str(),
                           element.retained);
    }
  }
}

Network::Network(DeviceState &device_state)
    : NetworkStateMachine("NetworkSM", *this),
      device_state_(device_state),
      mqtt_client_(wifi_client_) {
  WiFi.useStaticBuffers(true);
  mqtt_publish_queue_ = xQueueCreate(4, sizeof(PublishQueueElement));
  if (mqtt_publish_queue_ == nullptr) log_e("Failed to create publish queue");
}

Network::~Network() {
  if (mqtt_publish_queue_ != nullptr) {
    vQueueDelete(mqtt_publish_queue_);
  }
}

void Network::connect() {
  command_ = Command::CONNECT;
  this->erase_ = false;
  log_d("[NET] cmd connect");
}

void Network::disconnect(bool erase) {
  command_ = Command::DISCONNECT;
  this->erase_ = erase;
  log_d("[NET] cmd disconnect");
}

void Network::update() {
  mqtt_client_.loop();
  executeOnce();
}

void Network::setup() { network_task_handle_ = xTaskGetCurrentTaskHandle(); }

Network::State Network::get_state() { return state_; }

void Network::publish(const TopicType &topic, const PayloadType &payload,
                      bool retained) {
  auto current_task = xTaskGetCurrentTaskHandle();

  if (current_task == network_task_handle_) {
    log_d("publish from same task, no need to queue");
    _publish_unsafe(topic, payload.c_str(), retained);
  } else {
    PublishQueueElement element{topic, payload, retained};
    if (mqtt_publish_queue_ != nullptr &&
        xQueueSend(mqtt_publish_queue_, (void *)&element, (TickType_t)100)) {
      log_d("queue send successful (topic: %s)", topic.c_str());
    } else {
      log_e("queue send failed (topic: %s)", topic.c_str());
    }
  }
}

void Network::publish(const TopicType &topic, const char *payload,
                      bool retained) {
  publish(topic, PayloadType{payload}, retained);
}

bool Network::subscribe(const TopicType &topic) {
  if (xTaskGetCurrentTaskHandle() != network_task_handle_) {
    log_e("cannot subscribe from another task");
    return false;
  }
  if (topic.empty()) {
    log_w("[NET] sub to empty topic blocked", topic.c_str());
    return false;
  }
  bool ret;
  ret = mqtt_client_.subscribe(topic.c_str());
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
  usr_callback_ = callback;
}

void Network::set_on_connect(std::function<void()> on_connect) {
  this->on_connect_callback_ = on_connect;
}

bool Network::_connect_mqtt() {
  if (device_state_.userPreferences().mqtt.user.length() > 0 &&
      device_state_.userPreferences().mqtt.password.length() > 0) {
    return mqtt_client_.connect(
        device_state_.factory().unique_id.c_str(),
        device_state_.userPreferences().mqtt.user.c_str(),
        device_state_.userPreferences().mqtt.password.c_str());
  } else {
    return mqtt_client_.connect(device_state_.factory().unique_id.c_str());
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
  if (usr_callback_ != NULL) {
    usr_callback_(topic, buff);
  }
}

void Network::_publish_unsafe(const TopicType &topic, const char *payload,
                              bool retained) {
  bool ret;
  if (retained) {
    ret = mqtt_client_.publish(topic.c_str(), payload, true);
  } else {
    ret = mqtt_client_.publish(topic.c_str(), payload);
  }
  if (ret) {
    log_d("[NET] pub to: %s SUCCESS.", topic.c_str());
    log_d("[NET] content: %s", payload);
  } else {
    log_e("[NET] pub to: %s FAIL.", topic.c_str());
  }
}

#include "network.h"
#include <esp_wifi.h>
#include "config.h"
#include "state.h"
#include "utils.h"

static constexpr uint8_t MQTT_QUEUE_SIZE = 4;
static constexpr uint8_t MQTT_QUEUE_ITEMS_PER_LOOP = 5;

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

void NetworkSMStates::IdleState::loop() {
  if (sm().command_ == Network::Command::CONNECT) {
    if (sm().device_state_.persisted().wifi_quick_connect) {
      return transition_to<QuickConnectState>();
    } else {
      return transition_to<NormalConnectState>();
    }
  }
}

void NetworkSMStates::QuickConnectState::entry() {
  sm()._pre_wifi_connect();
  sm().info("connecting Wi-Fi (quick mode)...");
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);
  start_time_ = millis();
  WiFi.begin();
}

void NetworkSMStates::QuickConnectState::loop() {
  if (sm().command_ == Network::Command::DISCONNECT) {
    return transition_to<DisconnectState>();
  } else if (WiFi.status() == WL_CONNECTED) {
    sm().info("Wi-Fi connected (quick mode) in %lu ms.",
              millis() - start_time_);
    return transition_to<WifiConnectedState>();
  } else if (millis() - start_time_ > QUICK_WIFI_TIMEOUT) {
    // try again with normal mode
    sm().info(
        "Wi-Fi connect failed (quick mode). Retrying with normal "
        "mode...");
    sm().device_state_.persisted().wifi_quick_connect = false;
    return transition_to<DisconnectState>();
  }
}

void NetworkSMStates::NormalConnectState::entry() {
  sm()._pre_wifi_connect();
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);

  // get ssid from esp32 saved config
  wifi_config_t conf;
  if (esp_wifi_get_config(WIFI_IF_STA, &conf)) {
    sm().error("failed to get esp wifi config");
  }
  const char *ssid = reinterpret_cast<const char *>(conf.sta.ssid);
  const char *psk = reinterpret_cast<const char *>(conf.sta.password);
  sm().info("connecting Wi-Fi (normal mode): SSID: %s", ssid);
  WiFi.begin(ssid, psk);

  start_time_ = millis();
  await_confirm_quick_wifi_settings_ = false;
}

void NetworkSMStates::NormalConnectState::loop() {
  if (sm().command_ == Network::Command::DISCONNECT) {
    return transition_to<DisconnectState>();
  } else if (WiFi.status() == WL_CONNECTED) {
    if (await_confirm_quick_wifi_settings_) {
      sm().info("Wi-Fi connected, quick mode settings saved.");
      sm().device_state_.persisted().wifi_quick_connect = true;
      sm().device_state_.save_persisted();
      return transition_to<WifiConnectedState>();
    } else {
      // get bssid and ch, and save it directly to ESP
      sm().info(
          "Wi-Fi connected (normal mode) in %lu ms. Saving settings for quick "
          "mode...",
          millis() - start_time_);

      String ssid = WiFi.SSID();
      String psk = WiFi.psk();
      uint8_t *bssid = WiFi.BSSID();
      int32_t ch = WiFi.channel();
      sm().info("SSID: %s, BSSID: %s, CH: %d", ssid.c_str(),
                mac2String(bssid).c_str(), ch);

      // WiFi.disconnect(); not required, already done in WiFi.begin()
      WiFi.begin(ssid.c_str(), psk.c_str(), ch, bssid, true);
      start_time_ = millis();
      await_confirm_quick_wifi_settings_ = true;
    }
  } else if (millis() - start_time_ >= WIFI_TIMEOUT) {
    sm().warning("Wi-Fi connect failed (normal mode). Retrying...");
    return transition_to<DisconnectState>();
  }
}

void NetworkSMStates::MQTTConnectState::entry() {
  sm().mqtt_client_.setServer(
      sm().device_state_.user_preferences().mqtt.server.c_str(),
      sm().device_state_.user_preferences().mqtt.port);
  sm().mqtt_client_.setBufferSize(MQTT_BUFFER_SIZE);
  sm().mqtt_client_.setCallback(
      std::bind(&Network::_mqtt_callback, &sm(), std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3));
  // proceed with MQTT connection
  start_time_ = millis();
  sm().info("connecting MQTT....");
  sm()._connect_mqtt();
}

void NetworkSMStates::MQTTConnectState::loop() {
  if (sm().command_ == Network::Command::DISCONNECT) {
    return transition_to<DisconnectState>();
  } else if (sm().mqtt_client_.connected()) {
    sm().info("MQTT connected in %lu ms.", millis() - start_time_);
    sm().info("Network connected in %lu ms.",
              millis() - sm().cmd_connect_time_);
    return transition_to<FullyConnectedState>();
  } else if (millis() - start_time_ > MQTT_TIMEOUT) {
    if (WiFi.status() == WL_CONNECTED) {
      sm().state_ = Network::State::W_CONNECTED;
      sm().warning("MQTT connect failed. Retrying...");
      sm()._connect_mqtt();
      start_time_ = millis();
    } else {
      sm().warning(
          "MQTT connect failed. Wi-Fi not connected. Retrying "
          "Wi-Fi...");
      return transition_to<DisconnectState>();
    }
  }
}

void NetworkSMStates::WifiConnectedState::loop() {
  sm().state_ = Network::State::W_CONNECTED;
  sm().device_state_.set_ip(WiFi.localIP());
  sm().info("Wi-Fi connected.");
  sm().info("IP: %s", ip_address_to_static_string(WiFi.localIP()).c_str());
  String ssid = WiFi.SSID();
  sm().device_state_.save_all();
  uint8_t *bssid = WiFi.BSSID();
  int32_t ch = WiFi.channel();
  sm().info("SSID: %s, BSSID: %s, CH: %d", ssid.c_str(),
            mac2String(bssid).c_str(), ch);
  return transition_to<MQTTConnectState>();
}

void NetworkSMStates::DisconnectState::entry() {
  sm().info("disconnecting...");
  sm().mqtt_client_.disconnect();
  sm().wifi_client_.flush();
  WiFi.disconnect(true, sm().erase_);
  WiFi.mode(WIFI_OFF);
  sm().state_ = Network::State::DISCONNECTED;
  sm().info("disconnected.");
  delay(MQTT_DISCONNECT_TIMEOUT);
}

void NetworkSMStates::DisconnectState::loop() {
  return transition_to<IdleState>();
}

void NetworkSMStates::FullyConnectedState::entry() {
  last_conn_check_time_ = millis();
  if (sm().on_connect_callback_) {
    sm().on_connect_callback_();
  }
  sm().state_ = Network::State::M_CONNECTED;
}

void NetworkSMStates::FullyConnectedState::loop() {
  if (sm().command_ == Network::Command::DISCONNECT &&
      uxQueueMessagesWaiting(sm().mqtt_publish_queue_) == 0) {
    return transition_to<DisconnectState>();
  } else if (millis() - last_conn_check_time_ > NET_CONN_CHECK_INTERVAL) {
    if (WiFi.status() != WL_CONNECTED) {
      sm().warning("Wi-Fi connection interrupted. Reconnecting...");
      return transition_to<DisconnectState>();
    } else if (!sm().mqtt_client_.connected()) {
      sm().state_ = Network::State::W_CONNECTED;
      sm().warning("MQTT connection interrupted. Reconnecting...");
      return transition_to<MQTTConnectState>();
    }
    last_conn_check_time_ = millis();
  } else {
    SM::PublishQueueElement element;
    uint8_t max_element_to_process = MQTT_QUEUE_ITEMS_PER_LOOP;
    while (max_element_to_process > 0 && sm().mqtt_publish_queue_ != nullptr &&
           xQueueReceive(sm().mqtt_publish_queue_, &element, 0)) {
      sm().debug("received payload (topic: %s)", element.topic.c_str());
      sm()._publish_unsafe(element.topic, element.payload.c_str(),
                           element.retained);
      max_element_to_process--;
    }
  }
}

Network::Network(DeviceState &device_state)
    : NetworkStateMachine("NetworkSM", *this),
      Logger("NET"),
      device_state_(device_state),
      mqtt_client_(wifi_client_) {
  mqtt_publish_queue_ =
      xQueueCreate(MQTT_QUEUE_SIZE, sizeof(PublishQueueElement));
  if (mqtt_publish_queue_ == nullptr) error("Failed to create publish queue");
}

Network::~Network() {
  if (mqtt_publish_queue_ != nullptr) {
    vQueueDelete(mqtt_publish_queue_);
  }
}

void Network::connect() {
  command_ = Command::CONNECT;
  cmd_connect_time_ = millis();
  this->erase_ = false;
  debug("cmd connect");
}

void Network::disconnect(bool erase) {
  command_ = Command::DISCONNECT;
  this->erase_ = erase;
  debug("cmd disconnect");
}

void Network::update() {
  mqtt_client_.loop();
  loop();
}

void Network::setup() { network_task_handle_ = xTaskGetCurrentTaskHandle(); }

Network::State Network::get_state() { return state_; }

void Network::publish(const TopicType &topic, const PayloadType &payload,
                      bool retained) {
  auto current_task = xTaskGetCurrentTaskHandle();

  if (current_task == network_task_handle_) {
    debug("publish from same task, no need to queue");
    _publish_unsafe(topic, payload.c_str(), retained);
  } else {
    PublishQueueElement element{topic, payload, retained};
    if (mqtt_publish_queue_ != nullptr &&
        xQueueSend(mqtt_publish_queue_, (void *)&element, (TickType_t)100)) {
      debug("queue send successful (topic: %s)", topic.c_str());
    } else {
      error("queue send failed (topic: %s)", topic.c_str());
    }
  }
}

void Network::publish(const TopicType &topic, const char *payload,
                      bool retained) {
  publish(topic, PayloadType{payload}, retained);
}

bool Network::subscribe(const TopicType &topic) {
  if (xTaskGetCurrentTaskHandle() != network_task_handle_) {
    error("cannot subscribe from another task");
    return false;
  }
  if (topic.empty()) {
    warning("sub to empty topic blocked");
    return false;
  }
  bool ret;
  ret = mqtt_client_.subscribe(topic.c_str());
  if (ret) {
    debug("sub to: %s SUCCESS.", topic.c_str());
  } else {
    warning("sub to: %s FAIL.", topic.c_str());
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

void Network::_pre_wifi_connect() {
  WiFi.useStaticBuffers(true);
  // set static IP
  const auto &static_ip = device_state_.user_preferences().network.static_ip;
  const auto &gateway = device_state_.user_preferences().network.gateway;
  const auto &subnet = device_state_.user_preferences().network.subnet;
  auto dns = device_state_.user_preferences().network.dns;
  auto dns2 = device_state_.user_preferences().network.dns2;
  bool ip_ok = static_ip != IPAddress(0, 0, 0, 0);
  bool gw_ok = gateway != IPAddress(0, 0, 0, 0);
  bool sn_ok = subnet != IPAddress(0, 0, 0, 0);
  if (dns == IPAddress(0, 0, 0, 0)) {
    dns = gateway;
  }
  if (dns2 == IPAddress(0, 0, 0, 0)) {
    dns2 = IPAddress(1, 1, 1, 1);
  }
  if (ip_ok && gw_ok && sn_ok) {
    info("Using static IP %s, Gateway %s, Subnet %s, DNS1 %s, DNS2 %s",
         ip_address_to_static_string(static_ip).c_str(),
         ip_address_to_static_string(gateway).c_str(),
         ip_address_to_static_string(subnet).c_str(),
         ip_address_to_static_string(dns).c_str(),
         ip_address_to_static_string(dns2).c_str());
    WiFi.config(static_ip, gateway, subnet, dns, dns2);
  } else {
    info("Using DHCP. Static IP not set or not valid.");
  }
}

bool Network::_connect_mqtt() {
  if (device_state_.user_preferences().mqtt.user.length() > 0 &&
      device_state_.user_preferences().mqtt.password.length() > 0) {
    return mqtt_client_.connect(
        device_state_.factory().unique_id.c_str(),
        device_state_.user_preferences().mqtt.user.c_str(),
        device_state_.user_preferences().mqtt.password.c_str());
  } else {
    return mqtt_client_.connect(device_state_.factory().unique_id.c_str());
  }
}

void Network::_mqtt_callback(const char *topic, uint8_t *payload,
                             uint32_t length) {
  char buff[length + 1];
  memcpy(buff, payload, (size_t)length);
  buff[length] = '\0';  // required so it can be converted to String
  debug("msg on topic: %s, len: %d, payload: %s", topic, length, buff);
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
    debug("pub to: %s SUCCESS.", topic.c_str());
    debug("content: %s", payload);
  } else {
    error("pub to: %s FAIL.", topic.c_str());
  }
}

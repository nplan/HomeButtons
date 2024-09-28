#ifndef HOMEBUTTONS_NETWORK_H
#define HOMEBUTTONS_NETWORK_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "state_machine.h"
#include "mqtt_helper.h"  // For TopicType
#include "freertos/queue.h"
#include "logger.h"
#include "state.h"

class DeviceState;
class Network;

namespace NetworkSMStates {
class IdleState : public State<Network> {
 public:
  using State<Network>::State;

  void loop() override;

  const char *get_name() override { return "IdleState"; }
};

class QuickConnectState : public State<Network> {
 public:
  using State<Network>::State;

  void entry() override;
  void loop() override;

  const char *get_name() override { return "QuickConnectState"; }

 private:
  uint32_t start_time_ = 0;
};

class NormalConnectState : public State<Network> {
 public:
  using State<Network>::State;

  void entry() override;
  void loop() override;

  const char *get_name() override { return "NormalConnectState"; }

 private:
  uint32_t start_time_ = 0;
  bool await_confirm_quick_wifi_settings_ = false;
};

class MQTTConnectState : public State<Network> {
 public:
  using State<Network>::State;

  void entry() override;
  void loop() override;

  const char *get_name() override { return "MQTTConnectState"; }

 private:
  uint32_t start_time_ = 0;
};

class WifiConnectedState : public State<Network> {
 public:
  using State<Network>::State;

  void loop() override;

  const char *get_name() override { return "WifiConnectedState"; }
};

class DisconnectState : public State<Network> {
 public:
  using State<Network>::State;

  void entry() override;
  void loop() override;

  const char *get_name() override { return "DisconnectState"; }
};

class FullyConnectedState : public State<Network> {
 public:
  using State<Network>::State;

  void entry() override;
  void loop() override;

  const char *get_name() override { return "FullyConnectedState"; }

 private:
  uint32_t last_conn_check_time_ = 0;
};
}  // namespace NetworkSMStates

class Network;

using NetworkStateMachine = StateMachine<
    Network, NetworkSMStates::IdleState, NetworkSMStates::QuickConnectState,
    NetworkSMStates::NormalConnectState, NetworkSMStates::MQTTConnectState,
    NetworkSMStates::WifiConnectedState, NetworkSMStates::DisconnectState,
    NetworkSMStates::FullyConnectedState>;

class Network : public NetworkStateMachine, public Logger {
 public:
  enum class State {
    DISCONNECTED,
    W_CONNECTED,
    M_CONNECTED,
  };

  enum class Command { NONE, CONNECT, DISCONNECT };

  explicit Network(DeviceState &device_state, TopicHelper &topics);
  Network(const Network &) = delete;
  ~Network();

  void connect();
  void disconnect(bool erase = false);
  void update();
  void setup();  // Warning: must be called from same task (thread) as update()

  State get_state();

  IPAddress get_ip() { return WiFi.localIP(); }

  int32_t get_rssi() { return WiFi.RSSI(); }

  void publish(const TopicType &topic, const PayloadType &payload,
               bool retained = false);
  void publish(const TopicType &topic, const char *payload,
               bool retained = false);
  bool subscribe(const TopicType &topic);
  void set_mqtt_callback(
      std::function<void(const char *, const char *)> callback);
  void set_on_connect(std::function<void()> on_connect);

 private:
  State state_ = State::DISCONNECTED;
  Command command_ = Command::NONE;
  uint32_t cmd_connect_time_ = 0;
  bool erase_ = false;

  DeviceState &device_state_;
  WiFiClient wifi_client_;
  PubSubClient mqtt_client_;
  TopicHelper &topics_;
  QueueHandle_t mqtt_publish_queue_ = nullptr;
  TaskHandle_t network_task_handle_ = nullptr;

  struct PublishQueueElement {
    TopicType topic;
    PayloadType payload;
    bool retained;
  };

  std::function<void(const char *, const char *)> usr_callback_;
  std::function<void()> on_connect_callback_;

  void _pre_wifi_connect();
  bool _connect_mqtt();
  void _mqtt_callback(const char *topic, uint8_t *payload, uint32_t length);
  void _publish_unsafe(const TopicType &topic, const char *payload,
                       bool retained = false);

  friend class NetworkSMStates::IdleState;
  friend class NetworkSMStates::QuickConnectState;
  friend class NetworkSMStates::NormalConnectState;
  friend class NetworkSMStates::MQTTConnectState;
  friend class NetworkSMStates::WifiConnectedState;
  friend class NetworkSMStates::DisconnectState;
  friend class NetworkSMStates::FullyConnectedState;
};

StaticIPConfig validate_static_ip_config(StaticIPConfig config);

#endif  // HOMEBUTTONS_NETWORK_H

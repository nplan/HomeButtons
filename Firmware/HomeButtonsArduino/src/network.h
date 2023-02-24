#ifndef HOMEBUTTONS_NETWORK_H
#define HOMEBUTTONS_NETWORK_H

#include <Arduino.h>

#include "StateMachine.h"

class DeviceState;
class Network;

namespace NetworkSMStates {
class IdleState : public State<Network> {
 public:
  using State<Network>::State;

  void executeOnce() override;

  const char *getName() override { return "IdleState"; }
};

class QuickConnectState : public State<Network> {
 public:
  using State<Network>::State;

  void entry() override;
  void executeOnce() override;

  const char *getName() override { return "QuickConnectState"; }

 private:
  uint32_t m_start_time = 0;
};

class NormalConnectState : public State<Network> {
 public:
  using State<Network>::State;

  void entry() override;
  void executeOnce() override;

  const char *getName() override { return "NormalConnectState"; }

 private:
  uint32_t m_start_time = 0;
  bool m_await_confirm_quick_wifi_settings = false;
};

class MQTTConnectState : public State<Network> {
 public:
  using State<Network>::State;

  void entry() override;
  void executeOnce() override;

  const char *getName() override { return "MQTTConnectState"; }

 private:
  uint32_t m_start_time = 0;
};

class WifiConnectedState : public State<Network> {
 public:
  using State<Network>::State;

  void executeOnce() override;

  const char *getName() override { return "WifiConnectedState"; }
};

class DisconnectState : public State<Network> {
 public:
  using State<Network>::State;

  void entry() override;
  void executeOnce() override;

  const char *getName() override { return "DisconnectState"; }
};

class FullyConnectedState : public State<Network> {
 public:
  using State<Network>::State;

  void entry() override;
  void executeOnce() override;

  const char *getName() override { return "FullyConnectedState"; }

 private:
  uint32_t m_last_conn_check_time = 0;
};
}  // namespace NetworkSMStates

using NetworkStateMachine = StateMachine<
    NetworkSMStates::IdleState, NetworkSMStates::QuickConnectState,
    NetworkSMStates::NormalConnectState, NetworkSMStates::MQTTConnectState,
    NetworkSMStates::WifiConnectedState, NetworkSMStates::DisconnectState,
    NetworkSMStates::FullyConnectedState>;

class Network : public NetworkStateMachine {
 public:
  enum class State {
    DISCONNECTED,
    W_CONNECTED,
    M_CONNECTED,
  };

  enum class Command { NONE, CONNECT, DISCONNECT };

  explicit Network(DeviceState &device_state);

  void connect();
  void disconnect(bool erase = false);
  void update();

  State get_state();

  bool publish(const char *topic, const char *payload, bool retained = false);
  bool subscribe(const String &topic);
  void set_mqtt_callback(
      std::function<void(const char *, const char *)> callback);
  void set_on_connect(std::function<void()> on_connect);

 private:
  State state_ = State::DISCONNECTED;
  Command command_ = Command::NONE;
  bool erase_ = false;

  DeviceState &device_state_;

  std::function<void(const char *, const char *)> usr_callback_;
  std::function<void()> on_connect_callback_;

  bool _connect_mqtt();
  void _mqtt_callback(const char *topic, uint8_t *payload, uint32_t length);

  friend class NetworkSMStates::IdleState;
  friend class NetworkSMStates::QuickConnectState;
  friend class NetworkSMStates::NormalConnectState;
  friend class NetworkSMStates::MQTTConnectState;
  friend class NetworkSMStates::WifiConnectedState;
  friend class NetworkSMStates::DisconnectState;
  friend class NetworkSMStates::FullyConnectedState;
};

#endif  // HOMEBUTTONS_NETWORK_H

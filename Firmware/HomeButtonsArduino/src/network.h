#ifndef A07A16C8_5CB9_4A22_B45F_B3A0069C5B9F
#define A07A16C8_5CB9_4A22_B45F_B3A0069C5B9F

#include <Arduino.h>


class Network {

  public:
    enum class State {
      DISCONNECTED,
      W_CONNECTED,
      M_CONNECTED,
    };

    enum class CMDState {
      NONE,
      CONNECT,
      DISCONNECT
    };

    void connect();
    void disconnect(bool erase = false);
    void update();

    State get_state();

    bool publish(const char* topic, const char* payload, bool retained = false);
    bool subscribe(String topic);
    void set_callback(std::function<void(String, String)> callback);
    void set_on_connect(std::function<void()> on_connect);

    uint32_t get_cmd_connect_time();
  
  private:
    State state = State::DISCONNECTED;
    CMDState cmd_state = CMDState::NONE;

    String wifi_ssid = "";
    String wifi_psk = "";

    uint32_t wifi_start_time = 0;
    uint32_t mqtt_start_time = 0;
    uint32_t last_conn_check_time = 0;
    uint32_t disconnect_start_time = 0;
    uint32_t cmd_connect_time = 0;

    bool erase = false;

    enum StateMachineState {
      NONE,
      IDLE,
      DELAY_AFTER_WIFI_NORMAL_BEGIN,
      AWAIT_QUICK_WIFI_CONNECTION,
      AWAIT_NORMAL_WIFI_CONNECTION,
      AWAIT_CONFIRM_QUICK_WIFI_SETTINGS,
      AWAIT_MQTT_CONNECTION,
      CONNECTED,
      AWAIT_DISCONNECTING_TIMEOUT,
      DISCONNECT
    };

    StateMachineState sm_state = IDLE;
    StateMachineState prev_sm_state = IDLE;

    std::function<void(String, String)> usr_callback = NULL;
    std::function<void()> on_connect = NULL;

    bool connect_mqtt();
    void callback(const char* topic, uint8_t* payload, uint32_t length);
};

extern Network network;

#endif /* A07A16C8_5CB9_4A22_B45F_B3A0069C5B9F */

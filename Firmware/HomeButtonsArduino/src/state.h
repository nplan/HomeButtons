#ifndef HOMEBUTTONS_STATE_H
#define HOMEBUTTONS_STATE_H

#include <Preferences.h>

#include "buttons.h"
#include "config.h"
#include "static_string.h"

class DeviceState {
 private:
  struct Factory {
    String serial_number = "";  // len = 8
    String random_id = "";      // len = 6
    String model_name = "";     // 1 < len < 20
    String model_id = "";       // len = 2
    String hw_version = "";     // len = 3
    String unique_id = "";      // len = 20
  } m_factory;

  struct UserPreferences {
    String device_name = "";
    char btn_labels[NUM_BUTTONS][BTN_LABEL_MAXLEN + 1] = {};
    uint16_t sensor_interval = 0;  // minutes

    struct {
      String server = "";
      int32_t port = 0;
      String user = "";
      String password = "";
      String base_topic = "";
      String discovery_prefix = "";
    } mqtt;
  } m_userPreferences;

  struct Persisted {
    // Vars
    bool low_batt_mode = false;
    bool wifi_done = false;
    bool setup_done = false;
    String last_sw_ver = "";
    bool user_awake_mode = false;

    // Flags
    bool wifi_quick_connect = false;
    bool charge_complete_showing = false;
    bool info_screen_showing = false;
    bool check_connection = false;
    uint8_t failed_connections = 0;
    bool restart_to_wifi_setup = false;
    bool restart_to_setup = false;
    bool send_discovery_config = false;
    bool silent_restart = false;
  } m_persisted;

  struct Flags {
    bool display_redraw = false;
    bool awake_mode = false;
  } m_flags;

  struct Sensors {
    float temperature = 0;
    float humidity = 0;
    uint8_t battery_pct = 0;
    bool charging = false;
    bool dc_connected = false;
    bool battery_present = false;
    bool battery_low = false;
  } m_sensors;

 public:
  DeviceState() = default;
  DeviceState(const DeviceState&) = delete;

  // Factory
  const Factory& factory() const { return m_factory; }
  void set_serial_number(const String& str) { m_factory.serial_number = str; }
  void set_random_id(const String& str) { m_factory.random_id = str; }
  void set_model_name(const String& str) { m_factory.model_name = str; }
  void set_model_id(const String& str) { m_factory.model_id = str; }
  void set_hw_version(const String& str) { m_factory.hw_version = str; }
  void set_unique_id(const String& str) { m_factory.unique_id = str; }

  void save_factory();
  void load_factory();
  void clear_factory();

  // User preferences
  const UserPreferences& userPreferences() const { return m_userPreferences; }
  void set_mqtt_parameters(const String& server, int32_t port,
                           const String& user, const String& password,
                           const String& base_topic,
                           const String& discovery_prefix) {
    m_userPreferences.mqtt = {server,   port,       user,
                              password, base_topic, discovery_prefix};
  }

  const String& device_name() const { return m_userPreferences.device_name; }
  void set_device_name(const String& device_name) {
    m_userPreferences.device_name = device_name;
  }
  uint16_t sensor_interval() const { return m_userPreferences.sensor_interval; }
  void set_sensor_interval(uint16_t interval_min) {
    m_userPreferences.sensor_interval = interval_min;
  }
  const char* get_btn_label(uint8_t i) const;
  void set_btn_label(uint8_t i, const char* label);

  void save_user();
  void load_user();
  void clear_user();

  // Others
  const Persisted& persisted() const { return m_persisted; }
  Persisted& persisted() { return m_persisted; }
  const Flags& flags() const { return m_flags; }
  Flags& flags() { return m_flags; }
  const Sensors& sensors() const { return m_sensors; }
  Sensors& sensors() { return m_sensors; }

  void save_persisted();
  void load_persisted();
  void clear_persisted();
  void clear_persisted_flags();

  void save_all();
  void load_all();
  void clear_all();

  // TODO: maybe should be somewhere else
  StaticString<32> get_ap_ssid() const {
    return StaticString<32>("HB-") + m_factory.random_id.c_str();
  }
  const char* get_ap_password() const { return SETUP_AP_PASSWORD; }

  void set_ip(const IPAddress& ip_address) {
    _ip_address.set("%u.%u.%u.%u", ip_address[0], ip_address[1], ip_address[2],
                    ip_address[3]);
  }

  const char* ip() const { return _ip_address.c_str(); }

 private:
  Preferences preferences;
  StaticString<15> _ip_address;
};

#endif  // HOMEBUTTONS_STATE_H

#ifndef HOMEBUTTONS_STATE_H
#define HOMEBUTTONS_STATE_H

#include <Preferences.h>

#include "config.h"
#include "types.h"
#include "logger.h"
#include "hardware.h"
#include <IPAddress.h>

struct StaticIPConfig {
  bool valid;
  IPAddress static_ip;
  IPAddress gateway;
  IPAddress subnet;
  IPAddress dns;
  IPAddress dns2;
};

class DeviceState : public Logger {
 private:
  struct Factory {
    SerialNumber serial_number;  // len = 8
    RandomID random_id;          // len = 6
    ModelName model_name;        // 1 <= len <= 20
    ModelID model_id;            // len = 2
    HWVersion hw_version;        // len = 3
    UniqueID unique_id;          // len = 21
  } factory_;

  struct UserPreferences {
    DeviceName device_name;
    ButtonLabel btn_labels[NUM_BUTTONS];
    uint16_t sensor_interval = 0;  // minutes
    bool use_fahrenheit = false;

    StaticIPConfig network;

    struct {
      String server = "";
      int32_t port = 0;
      String user = "";
      String password = "";
      String base_topic = "";
      String discovery_prefix = "";
    } mqtt;
  } user_preferences_;

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
    bool user_msg_showing = false;
    bool check_connection = false;
    uint8_t failed_connections = 0;
    bool restart_to_wifi_setup = false;
    bool restart_to_setup = false;
    bool send_discovery_config = false;
    bool silent_restart = false;
    bool download_mdi_icons = false;
    bool connect_on_restart = false;
  } persisted_;

  struct Flags {
    bool display_redraw = false;
    bool awake_mode = false;
    uint32_t schedule_wakeup_time = 0;
    uint32_t last_user_input_time = 0;
    bool keep_frontlight_on = false;
  } flags_;

  struct Sensors {
    float temperature = 0;
    float humidity = 0;
    uint8_t battery_pct = 0;
    float battery_voltage = 0;
    bool charging = false;
    bool dc_connected = false;
    bool battery_present = false;
    bool battery_low = false;
  } sensors_;

 public:
  DeviceState() : Logger("State") {}
  DeviceState(const DeviceState&) = delete;

  // Factory
  const Factory& factory() const { return factory_; }

  // User preferences
  const UserPreferences& user_preferences() const { return user_preferences_; }
  void set_mqtt_parameters(const String& server, int32_t port,
                           const String& user, const String& password,
                           const String& base_topic,
                           const String& discovery_prefix) {
    user_preferences_.mqtt = {server,   port,       user,
                              password, base_topic, discovery_prefix};
  }
  void set_static_ip_config(const IPAddress& static_ip,
                            const IPAddress& gateway, const IPAddress& subnet,
                            const IPAddress& dns = IPAddress(),
                            const IPAddress& dns2 = IPAddress()) {
    user_preferences_.network.valid = false;
    user_preferences_.network.static_ip = static_ip;
    user_preferences_.network.gateway = gateway;
    user_preferences_.network.subnet = subnet;
    user_preferences_.network.dns = dns;
    user_preferences_.network.dns2 = dns2;
  }

  const StaticIPConfig get_static_ip_config() const {
    return user_preferences_.network;
  }

  const DeviceName& device_name() const {
    return user_preferences_.device_name;
  }
  void set_device_name(const DeviceName& device_name) {
    user_preferences_.device_name = device_name;
  }
  uint16_t sensor_interval() const { return user_preferences_.sensor_interval; }
  void set_sensor_interval(uint16_t interval_min) {
    user_preferences_.sensor_interval = interval_min;
  }
  const ButtonLabel& get_btn_label(uint8_t i) const;
  void set_btn_label(uint8_t i, const char* label);

  bool get_use_fahrenheit() const { return user_preferences_.use_fahrenheit; }
  StaticString<1> get_temp_unit() const {
    return StaticString<1>(user_preferences_.use_fahrenheit ? "F" : "C");
  }
  void set_temp_unit(StaticString<1> unit) {
    bool f = unit == "F" || unit == "f";
    user_preferences_.use_fahrenheit = f;
  }

  void save_user();
  void load_user();
  void clear_user();

  // Others
  const Persisted& persisted() const { return persisted_; }
  Persisted& persisted() { return persisted_; }
  const Flags& flags() const { return flags_; }
  Flags& flags() { return flags_; }
  const Sensors& sensors() const { return sensors_; }
  Sensors& sensors() { return sensors_; }

  void save_persisted();
  void load_persisted();
  void clear_persisted();
  void clear_persisted_flags();

  void save_all();
  void load_all(HardwareDefinition& hw);
  void clear_all();

  size_t get_free_entries();

  // TODO: maybe should be somewhere else
  StaticString<32> get_ap_ssid() const {
    return StaticString<32>("HB-") + factory_.random_id.c_str();
  }
  const char* get_ap_password() const { return SETUP_AP_PASSWORD; }

  void set_ip(const IPAddress& ip_address) {
    ip_address_.set("%u.%u.%u.%u", ip_address[0], ip_address[1], ip_address[2],
                    ip_address[3]);
  }

  const char* ip() const { return ip_address_.c_str(); }

 private:
  void _load_factory(HardwareDefinition& hw);

  template <std::size_t MAX_SIZE>
  void _load_to_static_string(StaticString<MAX_SIZE>& destination,
                              const char* key, const char* defaultValue) {
    char buffer[MAX_SIZE + 1];  // +1 for '\0' at the end
    auto ret = preferences_.getString(key, buffer, MAX_SIZE + 1);
    if (ret == 0)
      destination.set(defaultValue);
    else
      destination.set(buffer);
  }

  void _load_to_ip_address(IPAddress& destination, const char* key,
                           const char* defaultValue);

  Preferences preferences_;
  StaticString<15> ip_address_;
};

#endif  // HOMEBUTTONS_STATE_H

#ifndef HOMEBUTTONS_STATE_H
#define HOMEBUTTONS_STATE_H

#include <Preferences.h>

#include "buttons.h"
#include "config.h"
#include "static_string.h"

class DeviceState {
 public:
  using SerialNumber = StaticString<8>;
  using RandomID = StaticString<6>;
  using ModelName = StaticString<20>;
  using ModelID = StaticString<2>;
  using HWVersion = StaticString<3>;
  using UniqueID = StaticString<20>;

  using DeviceName = StaticString<20>;
  using ButtonLabel = StaticString<BTN_LABEL_MAXLEN>;

 private:
  struct Factory {
    SerialNumber serial_number;  // len = 8
    RandomID random_id;          // len = 6
    ModelName model_name;        // 1 < len < 20
    ModelID model_id;            // len = 2
    HWVersion hw_version;        // len = 3
    UniqueID unique_id;          // len = 20
  } factory_;

  struct UserPreferences {
    DeviceName device_name;
    ButtonLabel btn_labels[NUM_BUTTONS];
    uint16_t sensor_interval = 0;  // minutes

    struct {
      String server = "";
      int32_t port = 0;
      String user = "";
      String password = "";
      String base_topic = "";
      String discovery_prefix = "";
    } mqtt;
  } userPreferences_;

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
  } persisted_;

  struct Flags {
    bool display_redraw = false;
    bool awake_mode = false;
  } flags_;

  struct Sensors {
    float temperature = 0;
    float humidity = 0;
    uint8_t battery_pct = 0;
    bool charging = false;
    bool dc_connected = false;
    bool battery_present = false;
    bool battery_low = false;
  } sensors_;

 public:
  DeviceState() = default;
  DeviceState(const DeviceState&) = delete;

  // Factory
  const Factory& factory() const { return factory_; }
  void set_serial_number(const SerialNumber& str) {
    factory_.serial_number = str;
  }
  void set_random_id(const RandomID& str) { factory_.random_id = str; }
  void set_model_name(const ModelName& str) { factory_.model_name = str; }
  void set_model_id(const ModelID& str) { factory_.model_id = str; }
  void set_hw_version(const HWVersion& str) { factory_.hw_version = str; }
  void set_unique_id(const UniqueID& str) { factory_.unique_id = str; }

  void save_factory();
  void load_factory();
  void clear_factory();

  // User preferences
  const UserPreferences& userPreferences() const { return userPreferences_; }
  void set_mqtt_parameters(const String& server, int32_t port,
                           const String& user, const String& password,
                           const String& base_topic,
                           const String& discovery_prefix) {
    userPreferences_.mqtt = {server,   port,       user,
                             password, base_topic, discovery_prefix};
  }

  const DeviceName& device_name() const { return userPreferences_.device_name; }
  void set_device_name(const DeviceName& device_name) {
    userPreferences_.device_name = device_name;
  }
  uint16_t sensor_interval() const { return userPreferences_.sensor_interval; }
  void set_sensor_interval(uint16_t interval_min) {
    userPreferences_.sensor_interval = interval_min;
  }
  const ButtonLabel& get_btn_label(uint8_t i) const;
  void set_btn_label(uint8_t i, const char* label);

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
  void load_all();
  void clear_all();

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
  template <std::size_t MAX_SIZE>
  void _load_to_static_string(StaticString<MAX_SIZE>& destination,
                              const char* key, const char* defaultValue) {
    char buffer[MAX_SIZE];
    auto ret = preferences_.getString(key, buffer, MAX_SIZE);
    if (ret == 0)
      destination.set("");
    else
      destination.set(buffer);
  }

  Preferences preferences_;
  StaticString<15> ip_address_;
};

#endif  // HOMEBUTTONS_STATE_H

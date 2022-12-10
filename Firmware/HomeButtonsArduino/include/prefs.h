#ifndef PREFS_H
#define PREFS_H

#include <Arduino.h>
#include <Preferences.h>

#include "config.h"

struct NetworkSettings {
  String wifi_ssid = "";
  String wifi_password = "";
  String mqtt_server = "";
  String mqtt_user = "";
  String mqtt_password = "";
  int32_t mqtt_port = 0;
};

struct FactorySettings {
  String serial_number = "";  // len = 8
  String random_id = "";      // len = 6
  String model_name = "";     // 1 < len < 20
  String model_id = "";       // len = 2
  String hw_version = "";     // len = 3
  String unique_id = "";      // len = 20
};

struct UserSettings {
  String device_name = "";
  String mqtt_server = "";
  uint32_t mqtt_port = 0;
  String mqtt_user = "";
  String mqtt_password = "";
  String base_topic = "";
  String discovery_prefix = "";
  String btn_1_label = "";
  String btn_2_label = "";
  String btn_3_label = "";
  String btn_4_label = "";
  String btn_5_label = "";
  String btn_6_label = "";
  uint16_t sensor_interval = 0;
  String get_btn_label(uint8_t i) {
    switch (i) {
      case 0:
        return btn_1_label;
      case 1:
        return btn_2_label;
      case 2:
        return btn_3_label;
      case 3:
        return btn_4_label;
      case 4:
        return btn_5_label;
      case 5:
        return btn_6_label;
      default:
        return "";
    }
  }
  void set_btn_label(uint8_t i, String label) {
    switch (i) {
      case 0:
        btn_1_label = label;
        break;
      case 1:
        btn_2_label = label;
        break;
      case 2:
        btn_3_label = label;
        break;
      case 3:
        btn_4_label = label;
        break;
      case 4:
        btn_5_label = label;
        break;
      case 5:
        btn_6_label = label;
        break;
    }
  }
};

struct PersistedVars {
  bool low_batt_mode = false;
  bool wifi_done = false;
  bool setup_done = false;
  bool wifi_quick_connect = false;
  bool info_screen_showing = false;
  bool charge_complete_showing = false;
  bool reset_to_setup = false;
  uint8_t failed_connections = 0;
  bool check_connection = false;
  String last_sw_ver = "";
};

struct Topics {
  String btn_press[6];
  String temperature = "";
  String humidity = "";
  String battery = "";
  String sensor_interval_cmd = "";
  String sensor_interval_state = "";
  String btn_label_state[6];
  String btn_label_cmd[6];
};

struct Flags {
  bool buttons_redraw = false;
};

// ------ settings ------
extern FactorySettings factory_s;
extern UserSettings user_s;
extern PersistedVars persisted_s;
extern Topics topic_s;
extern Flags flags_s;

void save_factory_settings(FactorySettings settings);
FactorySettings read_factory_settings();
void clear_factory_settings();

void save_user_settings(UserSettings settings);
UserSettings read_user_settings();
void clear_user_settings();

void save_persisted_vars(PersistedVars vars);
PersistedVars read_persisted_vars();
void clear_persisted_vars();

void clear_all_preferences();

#endif
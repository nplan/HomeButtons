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
  String button_1_text = "";
  String button_2_text = "";
  String button_3_text = "";
  String button_4_text = "";
  String button_5_text = "";
  String button_6_text = "";
  uint16_t sensor_interval = 0;
};

struct PersistedVars {
  bool low_batt_mode = false;
  bool wifi_done = false;
  bool setup_done = false;
  bool wifi_quick_connect = false;
  bool info_screen_showing = false;
  bool charge_complete_showing = false;
  bool reset_to_setup = false;
};

struct Topics {
  String button_1_press = "";
  String button_2_press = "";
  String button_3_press = "";
  String button_4_press = "";
  String button_5_press = "";
  String button_6_press = "";
  String temperature = "";
  String humidity = "";
  String battery = "";
  String sensor_interval_cmd = "";
  String sensor_interval_state = "";
};

// ------ settings ------
extern FactorySettings factory_s;
extern UserSettings user_s;
extern PersistedVars persisted_s;
extern Topics topic_s;

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
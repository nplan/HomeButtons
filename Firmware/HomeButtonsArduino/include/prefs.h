#ifndef PREFS_H
#define PREFS_H

#include <Arduino.h>
#include <Preferences.h>

// ------ defaults ------
const char DEVICE_NAME[] = "Home Buttons 001";
const uint16_t MQTT_PORT = 1883;
const char BASE_TOPIC[] = "homebuttons";
const char DISCOVERY_PREFIX[] = "homeassistant";
const char BTN_1_TXT[] = "B1";
const char BTN_2_TXT[] = "B2";
const char BTN_3_TXT[] = "B3";
const char BTN_4_TXT[] = "B4";
const char BTN_5_TXT[] = "B5";
const char BTN_6_TXT[] = "B6";

const char BTN_PRESS_PAYLOAD[] = "PRESS";
const uint32_t QUICK_WIFI_TIMEOUT = 5000L;
const uint32_t WIFI_TIMEOUT = 10000L;
const uint32_t MQTT_TIMEOUT = 10000L;
const uint32_t SHORT_PRESS_TIME = 100;
const uint32_t MEDIUM_PRESS_TIME = 2000L;
const uint32_t LONG_PRESS_TIME = 10000L;
const uint32_t EXTRA_LONG_PRESS_TIME = 20000L;
const uint32_t ULTRA_LONG_PRESS_TIME = 30000L;
const uint32_t CONFIG_TIMEOUT = 600; //s
const uint32_t MQTT_DISCONNECT_TIMEOUT = 1000;
const uint32_t INFO_SCREEN_DISP_TIME = 30000L; // ms
const uint32_t SENSOR_PUBLISH_TIME = 600000; // ms

struct FactorySettings {
  String serial_number = ""; // len = 8
  String random_id = ""; // len = 6
  String model_name = ""; // 1 < len < 20
  String model_id = ""; // len = 2
  String hw_version = ""; // len = 3
  String unique_id = ""; // len = 20
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
};

struct PersistedVars {
  bool low_batt_mode = false;
  bool wifi_done = false;
  bool setup_done = false;
  bool wifi_quick_connect = false;
  bool info_screen_showing = false;
  bool charge_complete_showing = false;
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

void save_topics(Topics topic_s);
Topics read_topics();
void clear_topics();

void clear_all_preferences();

#endif
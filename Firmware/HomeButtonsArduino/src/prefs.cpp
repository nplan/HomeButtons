#include "prefs.h"

// ------ settings ------
FactorySettings factory_s = {};
UserSettings user_s = {};
PersistedVars persisted_s = {};
Topics topic_s = {};

Preferences preferences;

void save_factory_settings(FactorySettings settings) {
    preferences.begin("factory", false);
    preferences.putString("serial_number", settings.serial_number);
    preferences.putString("random_id", settings.random_id);
    preferences.putString("model_name", settings.model_name);
    preferences.putString("model_id", settings.model_id);
    preferences.putString("hw_version", settings.hw_version);
    preferences.putString("unique_id", settings.unique_id);
    preferences.end();
}

FactorySettings read_factory_settings() {
    FactorySettings settings;
    preferences.begin("factory", true);
    settings.serial_number = preferences.getString("serial_number", "");
    settings.random_id = preferences.getString("random_id", "");
    settings.model_name = preferences.getString("model_name", "");
    settings.model_id = preferences.getString("model_id", "");
    settings.hw_version = preferences.getString("hw_version", "");
    settings.unique_id = preferences.getString("unique_id", "");
    preferences.end();
    return settings;
}

void clear_factory_settings() {
    preferences.begin("factory", false);
    preferences.clear();
    preferences.end();
}

void save_user_settings(UserSettings settings) {
    preferences.begin("user", false);
    preferences.putString("device_name", settings.device_name);
    preferences.putString("mqtt_srv", settings.mqtt_server);
    preferences.putUInt("mqtt_port", settings.mqtt_port);
    preferences.putString("mqtt_user", settings.mqtt_user);
    preferences.putString("mqtt_pass", settings.mqtt_password);
    preferences.putString("base_topic", settings.base_topic);
    preferences.putString("disc_prefix", settings.discovery_prefix);
    preferences.putString("btn1_txt", settings.button_1_text);
    preferences.putString("btn2_txt", settings.button_2_text);
    preferences.putString("btn3_txt", settings.button_3_text);
    preferences.putString("btn4_txt", settings.button_4_text);
    preferences.putString("btn5_txt", settings.button_5_text);
    preferences.putString("btn6_txt", settings.button_6_text);
    preferences.end();
}

UserSettings read_user_settings() {
    UserSettings settings;
    preferences.begin("user", true);
    settings.device_name = preferences.getString("device_name", DEVICE_NAME);
    settings.mqtt_server = preferences.getString("mqtt_srv", "");
    settings.mqtt_port = preferences.getUInt("mqtt_port", MQTT_PORT);
    settings.mqtt_user = preferences.getString("mqtt_user", "");
    settings.mqtt_password = preferences.getString("mqtt_pass", "");
    settings.base_topic = preferences.getString("base_topic", BASE_TOPIC);
    settings.discovery_prefix = preferences.getString("disc_prefix", DISCOVERY_PREFIX);
    settings.button_1_text = preferences.getString("btn1_txt", BTN_1_TXT);
    settings.button_2_text = preferences.getString("btn2_txt", BTN_2_TXT);
    settings.button_3_text = preferences.getString("btn3_txt", BTN_3_TXT);
    settings.button_4_text = preferences.getString("btn4_txt", BTN_4_TXT);
    settings.button_5_text = preferences.getString("btn5_txt", BTN_5_TXT);
    settings.button_6_text = preferences.getString("btn6_txt", BTN_6_TXT);
    preferences.end();
    return settings;
}

void clear_user_settings() {
    preferences.begin("user", false);
    preferences.clear();
    preferences.end();
}

void save_persisted_vars(PersistedVars vars) {
    preferences.begin("persisted", false);
    preferences.putBool("lb_mode", vars.low_batt_mode);
    preferences.putBool("wifi_done", vars.wifi_done);
    preferences.putBool("setup_done", vars.setup_done);
    preferences.putBool("wifi_qc", vars.wifi_quick_connect);
    preferences.end();
}

PersistedVars read_persisted_vars() {
    PersistedVars vars;
    preferences.begin("persisted", false);
    vars.low_batt_mode = preferences.getBool("lb_mode", false);
    vars.wifi_done = preferences.getBool("wifi_done", false);
    vars.setup_done = preferences.getBool("setup_done", false);
    vars.wifi_quick_connect = preferences.getBool("wifi_qc", false);
    preferences.end();
    return vars;
}

void clear_persisted_vars() {
    preferences.begin("persisted", false);
    preferences.clear();
    preferences.end();
}

void save_topics(Topics topic_s) {
    preferences.begin("topics", false);
    preferences.putString("btn1_p", topic_s.button_1_press);
    preferences.putString("btn2_p", topic_s.button_2_press);
    preferences.putString("btn3_p", topic_s.button_3_press);
    preferences.putString("btn4_p", topic_s.button_4_press);
    preferences.putString("btn5_p", topic_s.button_5_press);
    preferences.putString("btn6_p", topic_s.button_6_press);
    preferences.putString("tmp", topic_s.temperature);
    preferences.putString("humd", topic_s.humidity);
    preferences.putString("batt", topic_s.battery);
    preferences.end();
}

Topics read_topics() {
    Topics topic_s;
    preferences.begin("topics", true);
    topic_s.button_1_press = preferences.getString("btn1_p", "");
    topic_s.button_2_press = preferences.getString("btn2_p", "");
    topic_s.button_3_press = preferences.getString("btn3_p", "");
    topic_s.button_4_press = preferences.getString("btn4_p", "");
    topic_s.button_5_press = preferences.getString("btn5_p", "");
    topic_s.button_6_press = preferences.getString("btn6_p", "");
    topic_s.temperature = preferences.getString("tmp", "");
    topic_s.humidity = preferences.getString("humd", "");
    topic_s.battery = preferences.getString("batt", "");
    preferences.end();
    return topic_s;
}

void clear_topics() {
    preferences.begin("topics", false);
    preferences.clear();
    preferences.end();
}

void clear_all_preferences() {
    clear_user_settings();
    clear_persisted_vars();
    clear_topics();
}

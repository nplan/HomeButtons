#include "state.h"

#include "config.h"

void DeviceState::save_factory() {
  preferences_.begin("factory", false);
  preferences_.putString("serial_number", factory_.serial_number.c_str());
  preferences_.putString("random_id", factory_.random_id.c_str());
  preferences_.putString("model_name", factory_.model_name.c_str());
  preferences_.putString("model_id", factory_.model_id.c_str());
  preferences_.putString("hw_version", factory_.hw_version.c_str());
  preferences_.putString("unique_id", factory_.unique_id.c_str());
  preferences_.end();
}

void DeviceState::load_factory() {
  preferences_.begin("factory", true);
  _load_to_static_string(factory_.serial_number, "serial_number", "");
  _load_to_static_string(factory_.random_id, "random_id", "");
  _load_to_static_string(factory_.model_name, "model_name", "");
  _load_to_static_string(factory_.model_id, "model_id", "");
  _load_to_static_string(factory_.hw_version, "hw_version", "");
  _load_to_static_string(factory_.unique_id, "unique_id", "");
  preferences_.end();
}

void DeviceState::clear_factory() {
  preferences_.begin("factory", false);
  preferences_.clear();
  preferences_.end();
}

void DeviceState::save_user() {
  preferences_.begin("user", false);
  preferences_.putString("device_name", userPreferences_.device_name.c_str());
  preferences_.putString("mqtt_srv", userPreferences_.mqtt.server);
  preferences_.putUInt("mqtt_port", userPreferences_.mqtt.port);
  preferences_.putString("mqtt_user", userPreferences_.mqtt.user);
  preferences_.putString("mqtt_pass", userPreferences_.mqtt.password);
  preferences_.putString("base_topic", userPreferences_.mqtt.base_topic);
  preferences_.putString("disc_prefix", userPreferences_.mqtt.discovery_prefix);
  preferences_.putString("btn1_txt", userPreferences_.btn_labels[0].c_str());
  preferences_.putString("btn2_txt", userPreferences_.btn_labels[1].c_str());
  preferences_.putString("btn3_txt", userPreferences_.btn_labels[2].c_str());
  preferences_.putString("btn4_txt", userPreferences_.btn_labels[3].c_str());
  preferences_.putString("btn5_txt", userPreferences_.btn_labels[4].c_str());
  preferences_.putString("btn6_txt", userPreferences_.btn_labels[5].c_str());
  preferences_.putUInt("sen_itv", userPreferences_.sensor_interval);
  preferences_.end();
}

void DeviceState::load_user() {
  preferences_.begin("user", true);
  _load_to_static_string(
      userPreferences_.device_name, "device_name",
      (DeviceName{DEVICE_NAME_DFLT} + " " + factory_.random_id).c_str());
  userPreferences_.mqtt.server = preferences_.getString("mqtt_srv", "");
  userPreferences_.mqtt.port =
      preferences_.getUInt("mqtt_port", MQTT_PORT_DFLT);
  userPreferences_.mqtt.user = preferences_.getString("mqtt_user", "");
  userPreferences_.mqtt.password = preferences_.getString("mqtt_pass", "");
  userPreferences_.mqtt.base_topic =
      preferences_.getString("base_topic", BASE_TOPIC_DFLT);
  userPreferences_.mqtt.discovery_prefix =
      preferences_.getString("disc_prefix", DISCOVERY_PREFIX_DFLT);

  _load_to_static_string(userPreferences_.btn_labels[0], "btn1_txt",
                         BTN_1_LABEL_DFLT);
  _load_to_static_string(userPreferences_.btn_labels[1], "btn2_txt",
                         BTN_2_LABEL_DFLT);
  _load_to_static_string(userPreferences_.btn_labels[2], "btn3_txt",
                         BTN_3_LABEL_DFLT);
  _load_to_static_string(userPreferences_.btn_labels[3], "btn4_txt",
                         BTN_4_LABEL_DFLT);
  _load_to_static_string(userPreferences_.btn_labels[4], "btn5_txt",
                         BTN_5_LABEL_DFLT);
  _load_to_static_string(userPreferences_.btn_labels[5], "btn6_txt",
                         BTN_6_LABEL_DFLT);

  userPreferences_.sensor_interval =
      preferences_.getUInt("sen_itv", SEN_INTERVAL_DFLT);
  preferences_.end();
}

void DeviceState::clear_user() {
  preferences_.begin("user", false);
  preferences_.clear();
  preferences_.end();
}

void DeviceState::save_persisted() {
  preferences_.begin("persisted", false);
  preferences_.putBool("lb_mode", persisted_.low_batt_mode);
  preferences_.putBool("wifi_done", persisted_.wifi_done);
  preferences_.putBool("setup_done", persisted_.setup_done);
  preferences_.putString("last_sw", persisted_.last_sw_ver);
  preferences_.putBool("u_awake", persisted_.user_awake_mode);
  preferences_.putBool("wifi_qc", persisted_.wifi_quick_connect);
  preferences_.putBool("chg_cpt_shwn", persisted_.charge_complete_showing);
  preferences_.putBool("info_shwn", persisted_.info_screen_showing);
  preferences_.putBool("chk_conn", persisted_.check_connection);
  preferences_.putUInt("faild_cons", persisted_.failed_connections);
  preferences_.putBool("rst_to_w_stp", persisted_.restart_to_wifi_setup);
  preferences_.putBool("rst_to_stp", persisted_.restart_to_setup);
  preferences_.putBool("send_adisc", persisted_.send_discovery_config);
  preferences_.putBool("silent_rst", persisted_.silent_restart);
  preferences_.end();
}

void DeviceState::load_persisted() {
  preferences_.begin("persisted", false);
  persisted_.low_batt_mode = preferences_.getBool("lb_mode", false);
  persisted_.wifi_done = preferences_.getBool("wifi_done", false);
  persisted_.setup_done = preferences_.getBool("setup_done", false);
  persisted_.last_sw_ver = preferences_.getString("last_sw", "");
  persisted_.user_awake_mode = preferences_.getBool("u_awake", false);
  persisted_.wifi_quick_connect = preferences_.getBool("wifi_qc", false);
  persisted_.charge_complete_showing =
      preferences_.getBool("chg_cpt_shwn", false);
  persisted_.info_screen_showing = preferences_.getBool("info_shwn", false);
  persisted_.check_connection = preferences_.getBool("chk_conn", false);
  persisted_.failed_connections = preferences_.getUInt("faild_cons", 0);
  persisted_.restart_to_wifi_setup =
      preferences_.getBool("rst_to_w_stp", false);
  persisted_.restart_to_setup = preferences_.getBool("rst_to_stp", false);
  persisted_.send_discovery_config = preferences_.getBool("send_adisc", false);
  persisted_.silent_restart = preferences_.getBool("silent_rst", false);
  preferences_.end();
}

void DeviceState::clear_persisted() {
  preferences_.begin("persisted", false);
  preferences_.clear();
  preferences_.end();
}

void DeviceState::clear_persisted_flags() {
  persisted_.wifi_quick_connect = false;
  persisted_.charge_complete_showing = false;
  persisted_.info_screen_showing = false;
  persisted_.check_connection = false;
  persisted_.failed_connections = 0;
  persisted_.restart_to_wifi_setup = false;
  persisted_.restart_to_setup = false;
  persisted_.silent_restart = false;
  save_all();
}

void DeviceState::save_all() {
  debug("state save all");
  save_user();
  save_persisted();
}

void DeviceState::load_all() {
  debug("state load all");
  load_factory();
  load_user();
  load_persisted();
}

void DeviceState::clear_all() {
  debug("state clear all");
  clear_user();
  clear_persisted();
}

const ButtonLabel& DeviceState::get_btn_label(uint8_t i) const {
  static ButtonLabel noLabel;
  if (i < NUM_BUTTONS) {
    return userPreferences_.btn_labels[i];
  } else {
    return noLabel;
  }
}

void DeviceState::set_btn_label(uint8_t i, const char* label) {
  if (i < NUM_BUTTONS) {
    userPreferences_.btn_labels[i].set(label);
  }
}

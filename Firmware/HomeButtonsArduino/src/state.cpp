#include "state.h"
#include "utils.h"
#include "config.h"

void DeviceState::save_user() {
  preferences_.begin("user", false);
  preferences_.putString("device_name", user_preferences_.device_name.c_str());
  preferences_.putString("mqtt_srv", user_preferences_.mqtt.server);
  preferences_.putUInt("mqtt_port", user_preferences_.mqtt.port);
  preferences_.putString("mqtt_user", user_preferences_.mqtt.user);
  preferences_.putString("mqtt_pass", user_preferences_.mqtt.password);
  preferences_.putString("base_topic", user_preferences_.mqtt.base_topic);
  preferences_.putString("disc_prefix",
                         user_preferences_.mqtt.discovery_prefix);
  for (int i = 0; i < NUM_BUTTONS; i++) {
    preferences_.putString(StaticString<8>("btn%d_txt", i + 1).c_str(),
                           user_preferences_.btn_labels[i].c_str());
  }
  preferences_.putUInt("sen_itv", user_preferences_.sensor_interval);
  preferences_.putBool("use_f", user_preferences_.use_fahrenheit);
  preferences_.putString(
      "sta_ip",
      ip_address_to_static_string(user_preferences_.network.static_ip).c_str());
  preferences_.putString(
      "g_way",
      ip_address_to_static_string(user_preferences_.network.gateway).c_str());
  preferences_.putString(
      "s_net",
      ip_address_to_static_string(user_preferences_.network.subnet).c_str());
  preferences_.putString(
      "dns",
      ip_address_to_static_string(user_preferences_.network.dns).c_str());
  preferences_.putString(
      "dns2",
      ip_address_to_static_string(user_preferences_.network.dns2).c_str());
  preferences_.end();
}

void DeviceState::load_user() {
  preferences_.begin("user", true);
  _load_to_static_string(
      user_preferences_.device_name, "device_name",
      (DeviceName{DEVICE_NAME_DFLT} + " " + factory_.random_id).c_str());
  user_preferences_.mqtt.server = preferences_.getString("mqtt_srv", "");
  user_preferences_.mqtt.port =
      preferences_.getUInt("mqtt_port", MQTT_PORT_DFLT);
  user_preferences_.mqtt.user = preferences_.getString("mqtt_user", "");
  user_preferences_.mqtt.password = preferences_.getString("mqtt_pass", "");
  user_preferences_.mqtt.base_topic =
      preferences_.getString("base_topic", BASE_TOPIC_DFLT);
  user_preferences_.mqtt.discovery_prefix =
      preferences_.getString("disc_prefix", DISCOVERY_PREFIX_DFLT);

  for (int i = 0; i < NUM_BUTTONS; i++) {
    _load_to_static_string(
        user_preferences_.btn_labels[i],
        StaticString<8>("btn%d_txt", i + 1).c_str(),
#ifndef HOME_BUTTONS_MINI
        StaticString<8>("%s%d", BNT_LABEL_DFLT_PREFIX, i + 1).c_str(),
#else
        StaticString<16>("mdi:numeric-%d", i + 1).c_str()
#endif
    );
  }

  user_preferences_.sensor_interval =
      preferences_.getUInt("sen_itv", SEN_INTERVAL_DFLT);
  user_preferences_.use_fahrenheit = preferences_.getBool("use_f", false);

  _load_to_ip_address(user_preferences_.network.static_ip, "sta_ip", "0.0.0.0");
  _load_to_ip_address(user_preferences_.network.gateway, "g_way", "0.0.0.0");
  _load_to_ip_address(user_preferences_.network.subnet, "s_net", "0.0.0.0");
  _load_to_ip_address(user_preferences_.network.dns, "dns", "0.0.0.0");
  _load_to_ip_address(user_preferences_.network.dns2, "dns2", "0.0.0.0");

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
  preferences_.putBool("dl_mdi", persisted_.download_mdi_icons);
  preferences_.putBool("con_on_r", persisted_.connect_on_restart);
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
  persisted_.download_mdi_icons = preferences_.getBool("dl_mdi", false);
  persisted_.connect_on_restart = preferences_.getBool("con_on_r", false);
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
  persisted_.download_mdi_icons = false;
  persisted_.connect_on_restart = false;
  save_all();
}

void DeviceState::_load_factory(HardwareDefinition& hw) {
  factory_.serial_number = hw.get_serial_number();
  factory_.random_id = hw.get_random_id();
  factory_.model_name = hw.get_model_name();
  factory_.model_id = hw.get_model_id();
  factory_.hw_version = hw.get_hw_version();
  factory_.unique_id = hw.get_unique_id();
}

void DeviceState::save_all() {
  debug("state save all");
  save_user();
  save_persisted();
}

void DeviceState::load_all(HardwareDefinition& hw) {
  debug("state load all");
  _load_factory(hw);
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
    return user_preferences_.btn_labels[i];
  } else {
    return noLabel;
  }
}

void DeviceState::set_btn_label(uint8_t i, const char* label) {
  if (i < NUM_BUTTONS) {
    user_preferences_.btn_labels[i].set(label);
  }
}

void DeviceState::_load_to_ip_address(IPAddress& destination, const char* key,
                                      const char* defaultValue) {
  char buffer[16];
  auto ret = preferences_.getString(key, buffer, 16);
  if (ret == 0)
    destination.fromString(defaultValue);
  else
    destination.fromString(buffer);
}
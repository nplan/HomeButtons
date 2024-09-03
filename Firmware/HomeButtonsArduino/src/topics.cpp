#include "topics.h"

TopicType TopicHelper::get_button_topic(UserInput::Event event) const {
  if (event.btn_id < 1 || event.btn_id > NUM_BUTTONS) return {};

  if (event.type == UserInput::EventType::kClickSingle)
    return t_common() + "button_" + event.btn_id;
  else if (event.type == UserInput::EventType::kClickDouble)
    return t_common() + "button_" + event.btn_id + "_double";
  else if (event.type == UserInput::EventType::kClickTriple)
    return t_common() + "button_" + event.btn_id + "_triple";
  else if (event.type == UserInput::EventType::kClickQuad)
    return t_common() + "button_" + event.btn_id + "_quad";
  else if (event.type == UserInput::EventType::kSwitchOn)
    return t_common() + "switch_" + event.btn_id;
  else if (event.type == UserInput::EventType::kSwitchOff)
    return t_common() + "switch_" + event.btn_id;
  else
    return {};
}

TopicType TopicHelper::t_switch_state(uint8_t switch_idx) const {
  if (switch_idx <= NUM_BUTTONS)
    return t_common() + "switch_" + (switch_idx);
  else
    return {};
}

TopicType TopicHelper::t_switch_cmd(uint8_t switch_idx) const {
  if (switch_idx <= NUM_BUTTONS)
    return t_cmd() + "switch_" + (switch_idx);
  else
    return {};
}

TopicType TopicHelper::t_common() const {
  return TopicType(_device_state.user_preferences().mqtt.base_topic.c_str()) +
         "/" + _device_state.user_preferences().device_name.c_str() + "/";
}

TopicType TopicHelper::t_cmd() const { return t_common() + "cmd/"; }
TopicType TopicHelper::t_temperature() const {
  return t_common() + "temperature";
}
TopicType TopicHelper::t_humidity() const { return t_common() + "humidity"; }
TopicType TopicHelper::t_battery() const { return t_common() + "battery"; }
TopicType TopicHelper::t_btn_press(uint8_t btn_idx) const {
  if (btn_idx <= NUM_BUTTONS)
    return t_common() + "button_" + (btn_idx);
  else
    return {};
}

TopicType TopicHelper::t_btn_label_state(uint8_t btn_idx) const {
  if (btn_idx <= NUM_BUTTONS)
    return t_common() + "btn_" + (btn_idx) + "_label";
  else
    return {};
}

TopicType TopicHelper::t_btn_label_cmd(uint8_t btn_idx) const {
  if (btn_idx <= NUM_BUTTONS)
    return t_cmd() + "btn_" + (btn_idx) + "_label";
  else
    return {};
}

TopicType TopicHelper::t_sensor_interval_state() const {
  return t_common() + "sensor_interval";
}

TopicType TopicHelper::t_sensor_interval_cmd() const {
  return t_cmd() + "sensor_interval";
}

TopicType TopicHelper::t_awake_mode_state() const {
  return t_common() + "awake_mode";
}

TopicType TopicHelper::t_awake_mode_cmd() const {
  return t_cmd() + "awake_mode";
}

TopicType TopicHelper::t_awake_mode_avlb() const {
  return t_awake_mode_state() + "/available";
}

TopicType TopicHelper::t_disp_msg_cmd() const { return t_cmd() + "disp_msg"; }

TopicType TopicHelper::t_disp_msg_state() const {
  return t_common() + "disp_msg";
}

TopicType TopicHelper::t_schedule_wakeup_cmd() const {
  return t_cmd() + "schedule_wakeup";
}

TopicType TopicHelper::t_schedule_wakeup_state() const {
  return t_common() + "schedule_wakeup";
}

TopicType TopicHelper::t_led_brightness_cmd() const {
  return t_cmd() + "led_brightness";
}

TopicType TopicHelper::t_led_brightness_state() const {
  return t_common() + "led_brightness";
}

TopicType TopicHelper::t_avlb() const { return t_common() + "available"; }

TopicType TopicHelper::t_btn_config(uint8_t btn_id) {
  return TopicType(
      "%s/device_automation/%s/button_%d/config",
      _device_state.user_preferences().mqtt.discovery_prefix.c_str(),
      _device_state.factory().unique_id.c_str(), btn_id);
}

TopicType TopicHelper::t_btn_double_config(uint8_t btn_id) {
  return TopicType(
      "%s/device_automation/%s/button_%d_double/config",
      _device_state.user_preferences().mqtt.discovery_prefix.c_str(),
      _device_state.factory().unique_id.c_str(), btn_id);
}

TopicType TopicHelper::t_btn_triple_config(uint8_t btn_id) {
  return TopicType(
      "%s/device_automation/%s/button_%d_triple/config",
      _device_state.user_preferences().mqtt.discovery_prefix.c_str(),
      _device_state.factory().unique_id.c_str(), btn_id);
}

TopicType TopicHelper::t_btn_quad_config(uint8_t btn_id) {
  return TopicType(
      "%s/device_automation/%s/button_%d_quad/config",
      _device_state.user_preferences().mqtt.discovery_prefix.c_str(),
      _device_state.factory().unique_id.c_str(), btn_id);
}

TopicType TopicHelper::t_switch_config(uint8_t switch_id) {
  return TopicType(
      "%s/switch/%s/switch_%d/config",
      _device_state.user_preferences().mqtt.discovery_prefix.c_str(),
      _device_state.factory().unique_id.c_str(), switch_id);
}

TopicType TopicHelper::t_kill_switch_config(uint8_t switch_id) {
  return TopicType(
      "%s/binary_sensor/%s/kill_switch_%d/config",
      _device_state.user_preferences().mqtt.discovery_prefix.c_str(),
      _device_state.factory().unique_id.c_str(), switch_id);
}

TopicType TopicHelper::t_temperature_config() {
  return TopicType(
      "%s/sensor/%s/temperature/config",
      _device_state.user_preferences().mqtt.discovery_prefix.c_str(),
      _device_state.factory().unique_id.c_str());
}

TopicType TopicHelper::t_humidity_config() {
  return TopicType(
      "%s/sensor/%s/humidity/config",
      _device_state.user_preferences().mqtt.discovery_prefix.c_str(),
      _device_state.factory().unique_id.c_str());
}

TopicType TopicHelper::t_sensor_interval_config() {
  return TopicType(
      "%s/number/%s/sensor_interval/config",
      _device_state.user_preferences().mqtt.discovery_prefix.c_str(),
      _device_state.factory().unique_id.c_str());
}

TopicType TopicHelper::t_battery_config() {
  return TopicType(
      "%s/sensor/%s/battery/config",
      _device_state.user_preferences().mqtt.discovery_prefix.c_str(),
      _device_state.factory().unique_id.c_str());
}

TopicType TopicHelper::t_btn_label_config(uint8_t btn_idx) {
  return TopicType(
      "%s/text/%s/button_%d_label/config",
      _device_state.user_preferences().mqtt.discovery_prefix.c_str(),
      _device_state.factory().unique_id.c_str(), btn_idx + 1);
}

TopicType TopicHelper::t_user_message_config() {
  return TopicType(
      "%s/text/%s/user_message/config",
      _device_state.user_preferences().mqtt.discovery_prefix.c_str(),
      _device_state.factory().unique_id.c_str());
}

TopicType TopicHelper::t_schedule_wakeup_config() {
  return TopicType(
      "%s/number/%s/schedule_wakeup/config",
      _device_state.user_preferences().mqtt.discovery_prefix.c_str(),
      _device_state.factory().unique_id.c_str());
}

TopicType TopicHelper::t_awake_mode_config() {
  return TopicType(
      "%s/switch/%s/awake_mode/config",
      _device_state.user_preferences().mqtt.discovery_prefix.c_str(),
      _device_state.factory().unique_id.c_str());
}

TopicType TopicHelper::t_led_brightness_config() {
  return TopicType(
      "%s/number/%s/led_brightness/config",
      _device_state.user_preferences().mqtt.discovery_prefix.c_str(),
      _device_state.factory().unique_id.c_str());
}

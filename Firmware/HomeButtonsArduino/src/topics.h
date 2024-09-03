#ifndef HOMEBUTTONS_TOPICS_H
#define HOMEBUTTONS_TOPICS_H

#include "types.h"
#include "user_input.h"
#include "state.h"

class TopicHelper {
 public:
  TopicHelper(DeviceState& device_state) : _device_state(device_state) {}

  // btn_id [1:NUM_BUTTONS]
  TopicType get_button_topic(UserInput::Event event) const;

  // btn_idx [0:NUM_BUTTONS-1]
  TopicType t_common() const;
  TopicType t_cmd() const;
  TopicType t_temperature() const;
  TopicType t_humidity() const;
  TopicType t_battery() const;
  TopicType t_btn_press(uint8_t btn_idx) const;
  TopicType t_btn_label_state(uint8_t btn_idx) const;
  TopicType t_btn_label_cmd(uint8_t btn_idx) const;
  TopicType t_sensor_interval_state() const;
  TopicType t_sensor_interval_cmd() const;
  TopicType t_awake_mode_state() const;
  TopicType t_awake_mode_cmd() const;
  TopicType t_awake_mode_avlb() const;
  TopicType t_disp_msg_cmd() const;
  TopicType t_disp_msg_state() const;
  TopicType t_schedule_wakeup_cmd() const;
  TopicType t_schedule_wakeup_state() const;
  TopicType t_led_brightness_cmd() const;
  TopicType t_led_brightness_state() const;
  TopicType t_avlb() const;
  TopicType t_switch_state(uint8_t switch_idx) const;
  TopicType t_switch_cmd(uint8_t switch_idx) const;

  // config topics
  TopicType t_btn_config(uint8_t btn_id);
  TopicType t_btn_double_config(uint8_t btn_id);
  TopicType t_btn_triple_config(uint8_t btn_id);
  TopicType t_btn_quad_config(uint8_t btn_id);
  TopicType t_switch_config(uint8_t switch_id);
  TopicType t_kill_switch_config(uint8_t switch_id);
  TopicType t_temperature_config();
  TopicType t_humidity_config();
  TopicType t_sensor_interval_config();
  TopicType t_battery_config();
  TopicType t_btn_label_config(uint8_t btn_idx);
  TopicType t_user_message_config();
  TopicType t_schedule_wakeup_config();
  TopicType t_awake_mode_config();
  TopicType t_led_brightness_config();

 private:
  DeviceState& _device_state;
};

#endif  // HOMEBUTTONS_TOPICS_H
#ifndef HOMEBUTTONS_MQTTHELPER_H
#define HOMEBUTTONS_MQTTHELPER_H

#include "static_string.h"
#include "buttons.h"

class DeviceState;
class Network;

static constexpr uint16_t MQTT_PYLD_SIZE = 512;
static constexpr uint16_t MQTT_BUFFER_SIZE = 777;
static constexpr size_t MAX_TOPIC_LENGTH = 256;
using TopicType = StaticString<MAX_TOPIC_LENGTH>;
using PayloadType = StaticString<MQTT_PYLD_SIZE>;

class MQTTHelper {
 public:
  MQTTHelper(DeviceState& state, Network& network);
  void send_discovery_config();
  void update_discovery_config();

  // btn_id [1:NUM_BUTTONS]
  TopicType get_button_topic(ButtonEvent event) const;

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

 private:
  DeviceState& _device_state;
  Network& _network;
};

#endif  // HOMEBUTTONS_MQTTHELPER_H

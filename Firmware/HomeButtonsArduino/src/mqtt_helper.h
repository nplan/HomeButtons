#ifndef HOMEBUTTONS_MQTTHELPER_H
#define HOMEBUTTONS_MQTTHELPER_H

#include "static_string.h"
#include "types.h"
#include "user_input.h"
#include "topics.h"
#include "config.h"
#include "button_ui/btn_sw_led.h"

class DeviceState;
class Network;

class MQTTHelper {
 public:
  MQTTHelper(DeviceState& state, BtnSwLEDInput<NUM_BUTTONS>& bsl_input,
             Network& network, TopicHelper& topics)
      : _device_state(state),
        bsl_input_(bsl_input),
        _network(network),
        topics_(topics) {};
  void send_discovery_config();
  void update_discovery_config();
  void clear_discovery_config();

 private:
  DeviceState& _device_state;
  BtnSwLEDInput<NUM_BUTTONS>& bsl_input_;
  Network& _network;
  TopicHelper& topics_;
};

#endif  // HOMEBUTTONS_MQTTHELPER_H

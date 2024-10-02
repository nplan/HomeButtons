#include "mqtt_helper.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#include "config.h"
#include "network.h"
#include "state.h"
#include "hardware.h"
#include "static_string.h"

template <size_t SIZE>
bool convertToJson(const StaticString<SIZE>& src, JsonVariant dst) {
  return dst.set(
      const_cast<char*>(src.c_str()));  // Warning: use char*, not const char*
                                        // to force a copy in ArduinoJson.
}

using FormatterType = StaticString<64>;

void MQTTHelper::send_discovery_config() {
  // clear first
  clear_discovery_config();

  // device objects
  StaticJsonDocument<256> device_full;
  device_full["ids"][0] = _device_state.factory().unique_id;
  device_full["mdl"] = _device_state.factory().model_name;
  device_full["name"] = _device_state.device_name();
  device_full["sw"] = SW_VERSION;
  device_full["hw"] = _device_state.factory().hw_version;
  device_full["mf"] = MANUFACTURER;
  device_full["cu"] = StaticString<32>("http://%s", _device_state.ip());

  StaticJsonDocument<128> device_short;
  device_short["ids"][0] = _device_state.factory().unique_id;

  uint16_t expire_after = _device_state.sensor_interval() * 60 + 60;  // seconds

  char buffer[MQTT_PYLD_SIZE];

  bool full_device_sent = false;

#if defined(HAS_BUTTON_UI)
  for (auto bsl_w : bsl_input_.GetBtnSwLEDs()) {
    auto bsl = bsl_w.get();
    if (!bsl.switch_mode()) {
      // button single press
      {
        StaticJsonDocument<MQTT_PYLD_SIZE> conf;
        conf["atype"] = "trigger";
        conf["t"] = topics_.t_btn_press(bsl.id());
        conf["pl"] = BTN_PRESS_PAYLOAD;
        conf["type"] = "button_short_press";
        conf["stype"] = FormatterType("button_%d", bsl.id());
        if (!full_device_sent) {
          conf["dev"] = device_full;
          full_device_sent = true;
        } else {
          conf["dev"] = device_short;
        }
        serializeJson(conf, buffer, sizeof(buffer));
        _network.publish(topics_.t_btn_config(bsl.id()), buffer, true);
      }

      // button double press
      {
        StaticJsonDocument<MQTT_PYLD_SIZE> conf;
        conf["atype"] = "trigger";
        conf["t"] = topics_.t_btn_press(bsl.id()) + "_double";
        conf["pl"] = BTN_PRESS_PAYLOAD;
        conf["type"] = "button_double_press";
        conf["stype"] = FormatterType("button_%d", bsl.id());
        conf["dev"] = device_short;
        serializeJson(conf, buffer, sizeof(buffer));
        _network.publish(topics_.t_btn_double_config(bsl.id()), buffer, true);
      }

      // button triple press
      {
        StaticJsonDocument<MQTT_PYLD_SIZE> conf;
        conf["atype"] = "trigger";
        conf["t"] = topics_.t_btn_press(bsl.id()) + "_triple";
        conf["pl"] = BTN_PRESS_PAYLOAD;
        conf["type"] = "button_triple_press";
        conf["stype"] = FormatterType("button_%d", bsl.id());
        conf["dev"] = device_short;
        serializeJson(conf, buffer, sizeof(buffer));
        _network.publish(topics_.t_btn_triple_config(bsl.id()), buffer, true);
      }

      // button quad press
      {
        StaticJsonDocument<MQTT_PYLD_SIZE> conf;
        conf["atype"] = "trigger";
        conf["t"] = topics_.t_btn_press(bsl.id()) + "_quad";
        conf["pl"] = BTN_PRESS_PAYLOAD;
        conf["type"] = "button_quadruple_press";
        conf["stype"] = FormatterType("button_%d", bsl.id());
        conf["dev"] = device_short;
        serializeJson(conf, buffer, sizeof(buffer));
        _network.publish(topics_.t_btn_quad_config(bsl.id()), buffer, true);
      }
    } else {  // switch
      if (!bsl.is_kill_switch()) {
        StaticJsonDocument<MQTT_PYLD_SIZE> conf;
        conf["name"] = FormatterType("Switch %d", bsl.id());
        conf["uniq_id"] = FormatterType{} + _device_state.factory().unique_id +
                          "_switch_" + bsl.id();
        conf["stat_t"] = topics_.t_switch_state(bsl.id());
        conf["cmd_t"] = topics_.t_switch_cmd(bsl.id());
        conf["ic"] = "mdi:radiobox-marked";
        conf["avty_t"] = topics_.t_avlb();
        conf["dev"] = device_short;
        serializeJson(conf, buffer, sizeof(buffer));
        _network.publish(topics_.t_switch_config(bsl.id()), buffer, true);
      } else {
        StaticJsonDocument<MQTT_PYLD_SIZE> conf;
        conf["name"] = "Kill Switch";
        conf["uniq_id"] = FormatterType{} + _device_state.factory().unique_id +
                          "_kill_switch";
        conf["stat_t"] = topics_.t_switch_state(5);
        conf["ic"] = "mdi:mushroom";
        conf["avty_t"] = topics_.t_avlb();
        conf["dev"] = device_short;
        serializeJson(conf, buffer, sizeof(buffer));
        _network.publish(topics_.t_kill_switch_config(bsl.id()), buffer, true);
      }
    }
  }
#endif

#if defined(HAS_TH_SENSOR)
  {
    // temperature

    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["name"] = "Temperature";
    conf["uniq_id"] =
        FormatterType{} + _device_state.factory().unique_id + "_temperature";
    conf["stat_t"] = topics_.t_temperature();
    conf["dev_cla"] = "temperature";
    conf["unit_of_meas"] = _device_state.get_use_fahrenheit() ? "°F" : "°C";
    conf["exp_aft"] = expire_after;
    conf["dev"] = device_short;
    serializeJson(conf, buffer, sizeof(buffer));
    _network.publish(topics_.t_temperature_config(), buffer, true);
  }

  {
    // humidity
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["name"] = "Humidity";
    conf["uniq_id"] =
        FormatterType{} + _device_state.factory().unique_id + "_humidity";
    conf["stat_t"] = topics_.t_humidity();
    conf["dev_cla"] = "humidity";
    conf["unit_of_meas"] = "%";
    conf["exp_aft"] = expire_after;
    conf["dev"] = device_short;
    serializeJson(conf, buffer, sizeof(buffer));
    _network.publish(topics_.t_humidity_config(), buffer, true);
  }
#endif

#if defined(HAS_BATTERY)
  {
    // battery
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["name"] = "Battery";
    conf["uniq_id"] =
        FormatterType{} + _device_state.factory().unique_id + "_battery";
    conf["stat_t"] = topics_.t_battery();
    conf["dev_cla"] = "battery";
    conf["unit_of_meas"] = "%";
    conf["exp_aft"] = expire_after;
    conf["dev"] = device_short;
    serializeJson(conf, buffer, sizeof(buffer));
    _network.publish(topics_.t_battery_config(), buffer, true); 
  }
#endif

#if defined(HAS_TH_SENSOR)
  {
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["name"] = "Sensor interval";
    conf["uniq_id"] = FormatterType{} + _device_state.factory().unique_id +
                      "_sensor_interval";
    conf["cmd_t"] = topics_.t_sensor_interval_cmd();
    conf["stat_t"] = topics_.t_sensor_interval_state();
    conf["unit_of_meas"] = "min";
    conf["min"] = SEN_INTERVAL_MIN;
    conf["max"] = SEN_INTERVAL_MAX;
    conf["mode"] = "slider";
    conf["ic"] = "mdi:timer-sand";
    conf["ret"] = "true";
    conf["dev"] = device_short;
    serializeJson(conf, buffer, sizeof(buffer));
    _network.publish(topics_.t_sensor_interval_config(), buffer, true);
  }
#endif

#if defined(HAS_DISPLAY)
  // button labels
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["name"] = FormatterType{} + "Button " + (i + 1) + " label";
    conf["uniq_id"] = FormatterType{} + _device_state.factory().unique_id +
                      "_button_" + (i + 1) + "_label";
    conf["cmd_t"] = topics_.t_btn_label_cmd(i + 1);
    conf["stat_t"] = topics_.t_btn_label_state(i + 1);
    conf["max"] = BTN_LABEL_MAXLEN;
    conf["ic"] = FormatterType("mdi:numeric-%d-box", i + 1);
    conf["ret"] = "true";
    conf["dev"] = device_short;
    serializeJson(conf, buffer, sizeof(buffer));
    _network.publish(topics_.t_btn_label_config(i + 1), buffer, true);
  }

  {
    // user message
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["name"] = "Show message";
    conf["uniq_id"] =
        FormatterType{} + _device_state.factory().unique_id + "_user_message";
    conf["cmd_t"] = topics_.t_disp_msg_cmd();
    conf["stat_t"] = topics_.t_disp_msg_state();
    conf["max"] = USER_MSG_MAXLEN;
    conf["ic"] = "mdi:message-text";
    conf["ret"] = "true";
    conf["dev"] = device_short;
    serializeJson(conf, buffer, sizeof(buffer));
    _network.publish(topics_.t_user_message_config(), buffer, true);
  }
#endif

#if defined(HAS_SLEEP_MODE)
  {
    // schedule wakeup
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["name"] = "Schedule wakeup";
    conf["uniq_id"] = FormatterType{} + _device_state.factory().unique_id +
                      "_schedule_wakeup";
    conf["cmd_t"] = topics_.t_schedule_wakeup_cmd();
    conf["stat_t"] = topics_.t_schedule_wakeup_state();
    conf["unit_of_meas"] = "s";
    conf["min"] = SCHEDULE_WAKEUP_MIN;
    conf["max"] = SCHEDULE_WAKEUP_MAX;
    conf["mode"] = "box";
    conf["ic"] = "mdi:alarm";
    conf["ret"] = "true";
    conf["dev"] = device_short;
    serializeJson(conf, buffer, sizeof(buffer));
    _network.publish(topics_.t_schedule_wakeup_config(), buffer, true);
  }
#endif

#if defined(HAS_AWAKE_MODE)
  {
    // awake mode toggle
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["name"] = "Awake mode";
    conf["uniq_id"] =
        FormatterType{} + _device_state.factory().unique_id + "_awake_mode";
    conf["cmd_t"] = topics_.t_awake_mode_cmd();
    conf["stat_t"] = topics_.t_awake_mode_state();
    conf["ic"] = "mdi:coffee";
    conf["ret"] = "true";
    conf["avty_t"] = topics_.t_awake_mode_avlb();
    conf["dev"] = device_short;
    serializeJson(conf, buffer, sizeof(buffer));
    _network.publish(topics_.t_awake_mode_config(), buffer, true);
  }
#endif

#if defined(HOME_BUTTONS_INDUSTRIAL)
  {
    // led brightness slider
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["name"] = "LED brightness";
    conf["uniq_id"] =
        FormatterType{} + _device_state.factory().unique_id + "_led_amb_bright";
    conf["cmd_t"] = topics_.t_led_amb_bright_cmd();
    conf["stat_t"] = topics_.t_led_amb_bright_state();
    conf["avty_t"] = topics_.t_avlb();
    conf["unit_of_meas"] = "%";
    conf["min"] = 0;
    conf["max"] = LED_MAX_AMB_BRIGHT;
    conf["mode"] = "slider";
    conf["ic"] = "mdi:led-on";
    conf["dev"] = device_short;
    serializeJson(conf, buffer, sizeof(buffer));
    _network.publish(topics_.t_led_amb_bright_config(), buffer, true);
  }
#endif
}

void MQTTHelper::update_discovery_config() {
  StaticJsonDocument<128> device_short;
  device_short["ids"][0] = _device_state.factory().unique_id;

  char buffer[MQTT_PYLD_SIZE];

  uint16_t expire_after = _device_state.sensor_interval() * 60 + 60;  // seconds

#if defined(HAS_TH_SENSOR)
  {
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["name"] = "Temperature";
    conf["uniq_id"] =
        FormatterType{} + _device_state.factory().unique_id + "_temperature";
    conf["stat_t"] = topics_.t_temperature();
    conf["dev_cla"] = "temperature";
    conf["unit_of_meas"] = _device_state.get_use_fahrenheit() ? "°F" : "°C";
    conf["exp_aft"] = expire_after;
    conf["dev"] = device_short;
    serializeJson(conf, buffer, sizeof(buffer));
    _network.publish(topics_.t_temperature_config(), buffer, true);
  }

  {
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["name"] = "Humidity";
    conf["uniq_id"] =
        FormatterType{} + _device_state.factory().unique_id + "_humidity";
    conf["stat_t"] = topics_.t_humidity();
    conf["dev_cla"] = "humidity";
    conf["unit_of_meas"] = "%";
    conf["exp_aft"] = expire_after;
    conf["dev"] = device_short;
    serializeJson(conf, buffer, sizeof(buffer));
    _network.publish(topics_.t_humidity_config(), buffer, true);
  }
#endif

#if defined(HAS_BATTERY)
  {
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["name"] = "Battery";
    conf["uniq_id"] =
        FormatterType{} + _device_state.factory().unique_id + "_battery";
    conf["stat_t"] = topics_.t_battery();
    conf["dev_cla"] = "battery";
    conf["unit_of_meas"] = "%";
    conf["exp_aft"] = expire_after;
    conf["dev"] = device_short;
    serializeJson(conf, buffer, sizeof(buffer));
    _network.publish(topics_.t_battery_config(), buffer, true);
  }
#endif
}

void MQTTHelper::clear_discovery_config() {
  // Construct topics

  // Buffer for empty payload
  const char* empty_payload = "";

#if defined(HAS_BUTTON_UI)
  for (auto bsl_w : bsl_input_.GetBtnSwLEDs()) {
    auto bsl = bsl_w.get();
    _network.publish(topics_.t_btn_config(bsl.id()), empty_payload, true);
    _network.publish(topics_.t_btn_double_config(bsl.id()), empty_payload,
                     true);
    _network.publish(topics_.t_btn_triple_config(bsl.id()), empty_payload,
                     true);
    _network.publish(topics_.t_btn_quad_config(bsl.id()), empty_payload, true);
    _network.publish(topics_.t_switch_config(bsl.id()), empty_payload, true);
    _network.publish(topics_.t_kill_switch_config(bsl.id()), empty_payload,
                     true);
  }
#endif

#if defined(HAS_TH_SENSOR)
  _network.publish(topics_.t_temperature_config(), empty_payload, true);
  _network.publish(topics_.t_humidity_config(), empty_payload, true);
  _network.publish(topics_.t_sensor_interval_config(), empty_payload, true);
#endif

#if defined(HAS_BATTERY)
  _network.publish(topics_.t_battery_config(), empty_payload, true);
#endif

#if defined(HAS_DISPLAY)
  // button labels
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    _network.publish(topics_.t_btn_label_config(i + 1), empty_payload, true);
  }
  _network.publish(topics_.t_user_message_config(), empty_payload, true);
#endif

#if defined(HAS_SLEEP_MODE)

  _network.publish(topics_.t_schedule_wakeup_config(), empty_payload, true);
#endif

#if defined(HAS_AWAKE_MODE)
  _network.publish(topics_.t_awake_mode_config(), empty_payload, true);
#endif

#if defined(HOME_BUTTONS_INDUSTRIAL)
  _network.publish(topics_.t_led_amb_bright_config(), empty_payload, true);
#endif
}

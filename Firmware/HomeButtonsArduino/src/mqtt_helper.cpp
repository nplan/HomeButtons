#include "mqtt_helper.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#include "config.h"
#include "network.h"
#include "state.h"
#include "hardware.h"
#include "static_string.h"

bool convertToJson(const TopicType& src, JsonVariant dst) {
  return dst.set(src.c_str());
}

MQTTHelper::MQTTHelper(DeviceState& state, Network& network)
    : _device_state(state), _network(network) {}

void MQTTHelper::send_discovery_config() {
  // Construct topics

  TopicType trigger_topic_common(
      "%s/device_automation/%s",
      _device_state.userPreferences().mqtt.discovery_prefix.c_str(),
      _device_state.factory().unique_id.c_str());

  // sensor config topics
  String sensor_topic_common =
      _device_state.userPreferences().mqtt.discovery_prefix + "/sensor/" +
      _device_state.factory().unique_id;
  String temperature_config_topic = sensor_topic_common + "/temperature/config";
  String humidity_config_topic = sensor_topic_common + "/humidity/config";
  String battery_config_topic = sensor_topic_common + "/battery/config";

  // command config topics
  String sensor_interval_config_topic =
      _device_state.userPreferences().mqtt.discovery_prefix + "/number/" +
      _device_state.factory().unique_id + "/sensor_interval/config";
  String button_label_config_topics[NUM_BUTTONS];
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    button_label_config_topics[i] =
        _device_state.userPreferences().mqtt.discovery_prefix + "/text/" +
        _device_state.factory().unique_id + "/button_" + String(i + 1) +
        "_label/config";
  }
  String switch_topic_common =
      _device_state.userPreferences().mqtt.discovery_prefix + "/switch/" +
      _device_state.factory().unique_id;
  String awake_mode_config_topic = switch_topic_common + "/awake_mode/config";

  // device objects
  StaticJsonDocument<256> device_full;
  device_full["ids"][0] = _device_state.factory().unique_id;
  device_full["mdl"] = _device_state.factory().model_name;
  device_full["name"] = _device_state.device_name();
  device_full["sw"] = SW_VERSION;
  device_full["hw"] = _device_state.factory().hw_version;
  device_full["mf"] = MANUFACTURER;

  StaticJsonDocument<128> device_short;
  device_short["ids"][0] = _device_state.factory().unique_id;

  // send mqtt msg
  char buffer[MQTT_PYLD_SIZE];

  // button single press
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["atype"] = "trigger";
    conf["t"] = t_btn_press(i);
    conf["pl"] = BTN_PRESS_PAYLOAD;
    conf["type"] = "button_short_press";
    conf["stype"] = "button_" + String(i + 1);
    if (i == 0) {
      conf["dev"] = device_full;
    } else {
      conf["dev"] = device_short;
    }
    serializeJson(conf, buffer);
    TopicType topic_name("%s/button_%d/config", trigger_topic_common.c_str(),
                         i + 1);
    _network.publish(topic_name.c_str(), buffer, true);
  }

  // button double press
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["atype"] = "trigger";
    conf["t"] = t_btn_press(i) + "_double";
    conf["pl"] = BTN_PRESS_PAYLOAD;
    conf["type"] = "button_double_press";
    conf["stype"] = "button_" + String(i + 1);
    conf["dev"] = device_short;
    serializeJson(conf, buffer);
    TopicType topic_name("%s/button_%d_double/config",
                         trigger_topic_common.c_str(), i + 1);
    _network.publish(topic_name.c_str(), buffer, true);
  }

  // button triple press
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["atype"] = "trigger";
    conf["t"] = t_btn_press(i) + "_triple";
    conf["pl"] = BTN_PRESS_PAYLOAD;
    conf["type"] = "button_triple_press";
    conf["stype"] = "button_" + String(i + 1);
    conf["dev"] = device_short;
    serializeJson(conf, buffer);
    TopicType topic_name("%s/button_%d_triple/config",
                         trigger_topic_common.c_str(), i + 1);
    _network.publish(topic_name.c_str(), buffer, true);
  }

  // button quad press
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["atype"] = "trigger";
    conf["t"] = t_btn_press(i) + "_quad";
    conf["pl"] = BTN_PRESS_PAYLOAD;
    conf["type"] = "button_quadruple_press";
    conf["stype"] = "button_" + String(i + 1);
    conf["dev"] = device_short;
    serializeJson(conf, buffer);
    TopicType topic_name("%s/button_%d_quad/config",
                         trigger_topic_common.c_str(), i + 1);
    _network.publish(topic_name.c_str(), buffer, true);
  }

  uint16_t expire_after = _device_state.sensor_interval() * 60 + 60;  // seconds

  {
    StaticJsonDocument<MQTT_PYLD_SIZE> temp_conf;
    temp_conf["name"] = _device_state.device_name() + " Temperature";
    temp_conf["uniq_id"] = _device_state.factory().unique_id + "_temperature";
    temp_conf["stat_t"] = t_temperature();
    temp_conf["dev_cla"] = "temperature";
    temp_conf["unit_of_meas"] = "°C";
    temp_conf["exp_aft"] = expire_after;
    temp_conf["dev"] = device_short;
    serializeJson(temp_conf, buffer);
    _network.publish(temperature_config_topic.c_str(), buffer, true);
  }

  {
    StaticJsonDocument<MQTT_PYLD_SIZE> humidity_conf;
    humidity_conf["name"] = _device_state.device_name() + " Humidity";
    humidity_conf["uniq_id"] = _device_state.factory().unique_id + "_humidity";
    humidity_conf["stat_t"] = t_humidity();
    humidity_conf["dev_cla"] = "humidity";
    humidity_conf["unit_of_meas"] = "%";
    humidity_conf["exp_aft"] = expire_after;
    humidity_conf["dev"] = device_short;
    serializeJson(humidity_conf, buffer);
    _network.publish(humidity_config_topic.c_str(), buffer, true);
  }

  {
    StaticJsonDocument<MQTT_PYLD_SIZE> battery_conf;
    battery_conf["name"] = _device_state.device_name() + " Battery";
    battery_conf["uniq_id"] = _device_state.factory().unique_id + "_battery";
    battery_conf["stat_t"] = t_battery();
    battery_conf["dev_cla"] = "battery";
    battery_conf["unit_of_meas"] = "%";
    battery_conf["exp_aft"] = expire_after;
    battery_conf["dev"] = device_short;
    serializeJson(battery_conf, buffer);
    _network.publish(battery_config_topic.c_str(), buffer, true);
  }

  {
    StaticJsonDocument<MQTT_PYLD_SIZE> sensor_interval_conf;
    sensor_interval_conf["name"] =
        _device_state.device_name() + " Sensor Interval";
    sensor_interval_conf["uniq_id"] =
        _device_state.factory().unique_id + "_sensor_interval";
    sensor_interval_conf["cmd_t"] = t_sensor_interval_cmd();
    sensor_interval_conf["stat_t"] = t_sensor_interval_state();
    sensor_interval_conf["unit_of_meas"] = "min";
    sensor_interval_conf["min"] = SEN_INTERVAL_MIN;
    sensor_interval_conf["max"] = SEN_INTERVAL_MAX;
    sensor_interval_conf["mode"] = "slider";
    sensor_interval_conf["ic"] = "mdi:timer-sand";
    sensor_interval_conf["ret"] = "true";
    sensor_interval_conf["dev"] = device_short;
    serializeJson(sensor_interval_conf, buffer);
    _network.publish(sensor_interval_config_topic.c_str(), buffer, true);
  }

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["name"] =
        _device_state.device_name() + " Button " + String(i + 1) + " Label";
    conf["uniq_id"] = _device_state.factory().unique_id + "_button_" +
                      String(i + 1) + "_label";
    conf["cmd_t"] = t_btn_label_cmd(i);
    conf["stat_t"] = t_btn_label_state(i);
    conf["max"] = BTN_LABEL_MAXLEN;
    conf["ic"] = String("mdi:numeric-") + String(i + 1) + "-box";
    conf["ret"] = "true";
    conf["dev"] = device_short;
    serializeJson(conf, buffer);
    _network.publish(button_label_config_topics[i].c_str(), buffer, true);
  }

  {
    StaticJsonDocument<MQTT_PYLD_SIZE> awake_mode_conf;
    awake_mode_conf["name"] = _device_state.device_name() + " Awake Mode";
    awake_mode_conf["uniq_id"] =
        _device_state.factory().unique_id + "_awake_mode";
    awake_mode_conf["cmd_t"] = t_awake_mode_cmd();
    awake_mode_conf["stat_t"] = t_awake_mode_state();
    awake_mode_conf["ic"] = "mdi:coffee";
    awake_mode_conf["ret"] = "true";
    awake_mode_conf["avty_t"] = t_awake_mode_avlb();
    awake_mode_conf["dev"] = device_short;
    serializeJson(awake_mode_conf, buffer);
    _network.publish(awake_mode_config_topic.c_str(), buffer, true);
  }
}

void MQTTHelper::update_discovery_config() {
  // sensor config topics
  String sensor_topic_common =
      _device_state.userPreferences().mqtt.discovery_prefix + "/sensor/" +
      _device_state.factory().unique_id;
  String temperature_config_topic = sensor_topic_common + "/temperature/config";
  String humidity_config_topic = sensor_topic_common + "/humidity/config";
  String battery_config_topic = sensor_topic_common + "/battery/config";

  StaticJsonDocument<128> device_short;
  device_short["ids"][0] = _device_state.factory().unique_id;

  char buffer[MQTT_PYLD_SIZE];

  uint16_t expire_after = _device_state.sensor_interval() * 60 + 60;  // seconds

  {
    StaticJsonDocument<MQTT_PYLD_SIZE> temp_conf;
    temp_conf["name"] = _device_state.device_name() + " Temperature";
    temp_conf["uniq_id"] = _device_state.factory().unique_id + "_temperature";
    temp_conf["stat_t"] = t_temperature();
    temp_conf["dev_cla"] = "temperature";
    temp_conf["unit_of_meas"] = "°C";
    temp_conf["exp_aft"] = expire_after;
    temp_conf["dev"] = device_short;
    serializeJson(temp_conf, buffer);
    _network.publish(temperature_config_topic.c_str(), buffer, true);
  }

  {
    StaticJsonDocument<MQTT_PYLD_SIZE> humidity_conf;
    humidity_conf["name"] = _device_state.device_name() + " Humidity";
    humidity_conf["uniq_id"] = _device_state.factory().unique_id + "_humidity";
    humidity_conf["stat_t"] = t_humidity();
    humidity_conf["dev_cla"] = "humidity";
    humidity_conf["unit_of_meas"] = "%";
    humidity_conf["exp_aft"] = expire_after;
    humidity_conf["dev"] = device_short;
    serializeJson(humidity_conf, buffer);
    _network.publish(humidity_config_topic.c_str(), buffer, true);
  }

  {
    StaticJsonDocument<MQTT_PYLD_SIZE> battery_conf;
    battery_conf["name"] = _device_state.device_name() + " Battery";
    battery_conf["uniq_id"] = _device_state.factory().unique_id + "_battery";
    battery_conf["stat_t"] = t_battery();
    battery_conf["dev_cla"] = "battery";
    battery_conf["unit_of_meas"] = "%";
    battery_conf["exp_aft"] = expire_after;
    battery_conf["dev"] = device_short;
    serializeJson(battery_conf, buffer);
    _network.publish(battery_config_topic.c_str(), buffer, true);
  }
}

TopicType MQTTHelper::get_button_topic(uint8_t btn_id,
                                       Button::ButtonAction action) {
  if (btn_id < 1 || btn_id > NUM_BUTTONS) {
    return {};
  }
  String append;
  switch (action) {
    case Button::SINGLE:
      append = "";
      break;
    case Button::DOUBLE:
      append = "_double";
      break;
    case Button::TRIPLE:
      append = "_triple";
      break;
    case Button::QUAD:
      append = "_quad";
      break;
    default:
      return {};
  }

  return t_common() + "button_" + btn_id + append.c_str();
}

TopicType MQTTHelper::t_common() const {
  return TopicType(_device_state.userPreferences().mqtt.base_topic.c_str()) +
         "/" + _device_state.userPreferences().device_name.c_str() + "/";
}

TopicType MQTTHelper::t_cmd() const { return t_common() + "cmd/"; }
TopicType MQTTHelper::t_temperature() const {
  return t_common() + "temperature";
}
TopicType MQTTHelper::t_humidity() const { return t_common() + "humidity"; }
TopicType MQTTHelper::t_battery() const { return t_common() + "battery"; }
TopicType MQTTHelper::t_btn_press(uint8_t i) const {
  if (i < NUM_BUTTONS)
    return t_common() + "button_" + String(i + 1).c_str();
  else
    return {};
}

TopicType MQTTHelper::t_btn_label_state(uint8_t i) const {
  if (i < NUM_BUTTONS)
    return t_common() + "btn_" + String(i + 1).c_str() + "_label";
  else
    return {};
}

TopicType MQTTHelper::t_btn_label_cmd(uint8_t i) const {
  if (i < NUM_BUTTONS)
    return t_cmd() + "btn_" + (i + 1) + "_label";
  else
    return {};
}

TopicType MQTTHelper::t_sensor_interval_state() const {
  return t_common() + "sensor_interval";
}

TopicType MQTTHelper::t_sensor_interval_cmd() const {
  return t_cmd() + "sensor_interval";
}

TopicType MQTTHelper::t_awake_mode_state() const {
  return t_common() + "awake_mode";
}
TopicType MQTTHelper::t_awake_mode_cmd() const {
  return t_cmd() + "awake_mode";
}
TopicType MQTTHelper::t_awake_mode_avlb() const {
  return t_awake_mode_state() + "/available";
}
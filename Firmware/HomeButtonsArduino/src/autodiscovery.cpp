#include "autodiscovery.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#include "config.h"
#include "network.h"
#include "state.h"
#include "hardware.h"
#include "StaticString.h"

void send_discovery_config(const DeviceState& device_state, Network& network) {
  // Construct topics
  using TopicType = StaticString<128>;
  TopicType trigger_topic_common("%s/device_automation/%s", device_state.network().mqtt.discovery_prefix.c_str(), device_state.factory().unique_id.c_str());

  // sensor config topics
  String sensor_topic_common =
      device_state.network().mqtt.discovery_prefix + "/sensor/" + device_state.factory().unique_id;
  String temperature_config_topic = sensor_topic_common + "/temperature/config";
  String humidity_config_topic = sensor_topic_common + "/humidity/config";
  String battery_config_topic = sensor_topic_common + "/battery/config";

  // command config topics
  String sensor_interval_config_topic = device_state.network().mqtt.discovery_prefix +
                                        "/number/" + device_state.factory().unique_id +
                                        "/sensor_interval/config";
  String button_label_config_topics[NUM_BUTTONS];
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    button_label_config_topics[i] = device_state.network().mqtt.discovery_prefix + "/text/" +
                                    device_state.factory().unique_id + "/button_" +
                                    String(i + 1) + "_label/config";
  }
  String switch_topic_common =
      device_state.network().mqtt.discovery_prefix + "/switch/" + device_state.factory().unique_id;
  String awake_mode_config_topic = switch_topic_common + "/awake_mode/config";

  // device objects
  StaticJsonDocument<256> device_full;
  device_full["ids"][0] = device_state.factory().unique_id;
  device_full["mdl"] = device_state.factory().model_name;
  device_full["name"] = device_state.device_name();
  device_full["sw"] = SW_VERSION;
  device_full["hw"] = device_state.factory().hw_version;
  device_full["mf"] = MANUFACTURER;

  StaticJsonDocument<128> device_short;
  device_short["ids"][0] = device_state.factory().unique_id;

  // send mqtt msg
  size_t n;
  char buffer[MQTT_PYLD_SIZE];

  // button single press
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["atype"] = "trigger";
    conf["t"] = device_state.topics().t_btn_press[i];
    conf["pl"] = BTN_PRESS_PAYLOAD;
    conf["type"] = "button_short_press";
    conf["stype"] = "button_" + String(i + 1);
    if (i == 0) {
      conf["dev"] = device_full;
    } else {
      conf["dev"] = device_short;
    }
    n = serializeJson(conf, buffer);
    TopicType topic_name("%s/button_%d/config", trigger_topic_common.c_str(), i + 1);
    network.publish(topic_name.c_str(), buffer, true);
  }

  // button double press
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["atype"] = "trigger";
    conf["t"] = device_state.topics().t_btn_press[i] + "_double";
    conf["pl"] = BTN_PRESS_PAYLOAD;
    conf["type"] = "button_double_press";
    conf["stype"] = "button_" + String(i + 1);
    conf["dev"] = device_short;
    n = serializeJson(conf, buffer);
    TopicType topic_name("%s/button_%d_double/config", trigger_topic_common.c_str(), i + 1);
    network.publish(topic_name.c_str(), buffer, true);
  }

  // button triple press
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["atype"] = "trigger";
    conf["t"] = device_state.topics().t_btn_press[i] + "_triple";
    conf["pl"] = BTN_PRESS_PAYLOAD;
    conf["type"] = "button_triple_press";
    conf["stype"] = "button_" + String(i + 1);
    conf["dev"] = device_short;
    n = serializeJson(conf, buffer);
    TopicType topic_name("%s/button_%d_triple/config", trigger_topic_common.c_str(), i + 1);
    network.publish(topic_name.c_str(), buffer, true);
  }

  // button quad press
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["atype"] = "trigger";
    conf["t"] = device_state.topics().t_btn_press[i] + "_quad";
    conf["pl"] = BTN_PRESS_PAYLOAD;
    conf["type"] = "button_quadruple_press";
    conf["stype"] = "button_" + String(i + 1);
    conf["dev"] = device_short;
    n = serializeJson(conf, buffer);
    TopicType topic_name("%s/button_%d_quad/config", trigger_topic_common.c_str(), i + 1);
    network.publish(topic_name.c_str(), buffer, true);
  }

  uint16_t expire_after = device_state.sensor_interval() * 60 + 60;  // seconds

  {
    StaticJsonDocument<MQTT_PYLD_SIZE> temp_conf;
    temp_conf["name"] = device_state.device_name() + " Temperature";
    temp_conf["uniq_id"] = device_state.factory().unique_id + "_temperature";
    temp_conf["stat_t"] = device_state.topics().t_temperature;
    temp_conf["dev_cla"] = "temperature";
    temp_conf["unit_of_meas"] = "°C";
    temp_conf["exp_aft"] = expire_after;
    temp_conf["dev"] = device_short;
    n = serializeJson(temp_conf, buffer);
    network.publish(temperature_config_topic.c_str(), buffer, true);
  }

  {
    StaticJsonDocument<MQTT_PYLD_SIZE> humidity_conf;
    humidity_conf["name"] = device_state.device_name() + " Humidity";
    humidity_conf["uniq_id"] = device_state.factory().unique_id + "_humidity";
    humidity_conf["stat_t"] = device_state.topics().t_humidity;
    humidity_conf["dev_cla"] = "humidity";
    humidity_conf["unit_of_meas"] = "%";
    humidity_conf["exp_aft"] = expire_after;
    humidity_conf["dev"] = device_short;
    n = serializeJson(humidity_conf, buffer);
    network.publish(humidity_config_topic.c_str(), buffer, true);
  }

  {
    StaticJsonDocument<MQTT_PYLD_SIZE> battery_conf;
    battery_conf["name"] = device_state.device_name() + " Battery";
    battery_conf["uniq_id"] = device_state.factory().unique_id + "_battery";
    battery_conf["stat_t"] = device_state.topics().t_battery;
    battery_conf["dev_cla"] = "battery";
    battery_conf["unit_of_meas"] = "%";
    battery_conf["exp_aft"] = expire_after;
    battery_conf["dev"] = device_short;
    n = serializeJson(battery_conf, buffer);
    network.publish(battery_config_topic.c_str(), buffer, true);
  }

  {
    StaticJsonDocument<MQTT_PYLD_SIZE> sensor_interval_conf;
    sensor_interval_conf["name"] = device_state.device_name() + " Sensor Interval";
    sensor_interval_conf["uniq_id"] =
        device_state.factory().unique_id + "_sensor_interval";
    sensor_interval_conf["cmd_t"] = device_state.topics().t_sensor_interval_cmd;
    sensor_interval_conf["stat_t"] = device_state.topics().t_sensor_interval_state;
    sensor_interval_conf["unit_of_meas"] = "min";
    sensor_interval_conf["min"] = SEN_INTERVAL_MIN;
    sensor_interval_conf["max"] = SEN_INTERVAL_MAX;
    sensor_interval_conf["mode"] = "slider";
    sensor_interval_conf["ic"] = "mdi:timer-sand";
    sensor_interval_conf["ret"] = "true";
    sensor_interval_conf["dev"] = device_short;
    n = serializeJson(sensor_interval_conf, buffer);
    network.publish(sensor_interval_config_topic.c_str(), buffer, true);
  }

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["name"] =
        device_state.device_name() + " Button " + String(i + 1) + " Label";
    conf["uniq_id"] =
        device_state.factory().unique_id + "_button_" + String(i + 1) + "_label";
    conf["cmd_t"] = device_state.topics().t_btn_label_cmd[i];
    conf["stat_t"] = device_state.topics().t_btn_label_state[i];
    conf["max"] = BTN_LABEL_MAXLEN;
    conf["ic"] = String("mdi:numeric-") + String(i + 1) + "-box";
    conf["ret"] = "true";
    conf["dev"] = device_short;
    n = serializeJson(conf, buffer);
    network.publish(button_label_config_topics[i].c_str(), buffer, true);
  }

  {
    StaticJsonDocument<MQTT_PYLD_SIZE> awake_mode_conf;
    awake_mode_conf["name"] = device_state.device_name() + " Awake Mode";
    awake_mode_conf["uniq_id"] = device_state.factory().unique_id + "_awake_mode";
    awake_mode_conf["cmd_t"] = device_state.topics().t_awake_mode_cmd;
    awake_mode_conf["stat_t"] = device_state.topics().t_awake_mode_state;
    awake_mode_conf["ic"] = "mdi:coffee";
    awake_mode_conf["ret"] = "true";
    awake_mode_conf["avty_t"] = device_state.topics().t_awake_mode_avlb;
    awake_mode_conf["dev"] = device_short;
    n = serializeJson(awake_mode_conf, buffer);
    network.publish(awake_mode_config_topic.c_str(), buffer, true);
  }
}

void update_discovery_config(const DeviceState& device_state, Network& network) {
  // sensor config topics
  String sensor_topic_common =
      device_state.network().mqtt.discovery_prefix + "/sensor/" + device_state.factory().unique_id;
  String temperature_config_topic = sensor_topic_common + "/temperature/config";
  String humidity_config_topic = sensor_topic_common + "/humidity/config";
  String battery_config_topic = sensor_topic_common + "/battery/config";

  StaticJsonDocument<128> device_short;
  device_short["ids"][0] = device_state.factory().unique_id;

  size_t n;
  char buffer[MQTT_PYLD_SIZE];

  uint16_t expire_after = device_state.sensor_interval() * 60 + 60;  // seconds

  {
    StaticJsonDocument<MQTT_PYLD_SIZE> temp_conf;
    temp_conf["name"] = device_state.device_name() + " Temperature";
    temp_conf["uniq_id"] = device_state.factory().unique_id + "_temperature";
    temp_conf["stat_t"] = device_state.topics().t_temperature;
    temp_conf["dev_cla"] = "temperature";
    temp_conf["unit_of_meas"] = "°C";
    temp_conf["exp_aft"] = expire_after;
    temp_conf["dev"] = device_short;
    n = serializeJson(temp_conf, buffer);
    network.publish(temperature_config_topic.c_str(), buffer, true);
  }

  {
    StaticJsonDocument<MQTT_PYLD_SIZE> humidity_conf;
    humidity_conf["name"] = device_state.device_name() + " Humidity";
    humidity_conf["uniq_id"] = device_state.factory().unique_id + "_humidity";
    humidity_conf["stat_t"] = device_state.topics().t_humidity;
    humidity_conf["dev_cla"] = "humidity";
    humidity_conf["unit_of_meas"] = "%";
    humidity_conf["exp_aft"] = expire_after;
    humidity_conf["dev"] = device_short;
    n = serializeJson(humidity_conf, buffer);
    network.publish(humidity_config_topic.c_str(), buffer, true);
  }

  {
    StaticJsonDocument<MQTT_PYLD_SIZE> battery_conf;
    battery_conf["name"] = device_state.device_name() + " Battery";
    battery_conf["uniq_id"] = device_state.factory().unique_id + "_battery";
    battery_conf["stat_t"] = device_state.topics().t_battery;
    battery_conf["dev_cla"] = "battery";
    battery_conf["unit_of_meas"] = "%";
    battery_conf["exp_aft"] = expire_after;
    battery_conf["dev"] = device_short;
    n = serializeJson(battery_conf, buffer);
    network.publish(battery_config_topic.c_str(), buffer, true);
  }
}
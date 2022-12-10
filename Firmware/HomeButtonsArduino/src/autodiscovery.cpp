#include "autodiscovery.h"

#include "network.h"

void send_autodiscovery_msg() {
  // Construct topics
  String trigger_topic_common =
      user_s.discovery_prefix + "/device_automation/" + factory_s.unique_id;

  // button press config topics
  String button_press_config_topics[NUM_BUTTONS];
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    button_press_config_topics[i] =
        trigger_topic_common + "/button_" + String(i + 1) + "/config";
  }

  // sensor config topics
  String sensor_topic_common =
      user_s.discovery_prefix + "/sensor/" + factory_s.unique_id;
  String temperature_config_topic = sensor_topic_common + "/temperature/config";
  String humidity_config_topic = sensor_topic_common + "/humidity/config";
  String battery_config_topic = sensor_topic_common + "/battery/config";

  // command config topics
  String sensor_interval_config_topic = user_s.discovery_prefix + "/number/" +
                                        factory_s.unique_id +
                                        "/sensor_interval/config";
  String button_label_config_topics[NUM_BUTTONS];
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    button_label_config_topics[i] = user_s.discovery_prefix + "/text/" +
                                    factory_s.unique_id + "/button_" +
                                    String(i + 1) + "_label/config";
  }

  // device objects
  StaticJsonDocument<256> device_full;
  device_full["identifiers"][0] = factory_s.unique_id;
  device_full["model"] = factory_s.model_name;
  device_full["name"] = user_s.device_name;
  device_full["sw_version"] = SW_VERSION;
  device_full["hw_version"] = factory_s.hw_version;
  device_full["manufacturer"] = MANUFACTURER;

  StaticJsonDocument<128> device_short;
  device_short["identifiers"][0] = factory_s.unique_id;
  

  // button press
  DynamicJsonDocument *btn_press_configs[NUM_BUTTONS];
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    btn_press_configs[i] = new DynamicJsonDocument(2048);
    DynamicJsonDocument *conf = btn_press_configs[i];
    (*conf)["automation_type"] = "trigger";
    (*conf)["topic"] = topic_s.btn_press[i];
    (*conf)["payload"] = BTN_PRESS_PAYLOAD;
    (*conf)["type"] = "button_short_press";
    (*conf)["subtype"] = "button_" + String(i + 1);
    if (i == 0) {
      (*conf)["device"] = device_full;
    } else {
      (*conf)["device"] = device_short;
    }
  }

  DynamicJsonDocument temp_conf(2048);
  temp_conf["name"] = user_s.device_name + " Temperature";
  temp_conf["unique_id"] = factory_s.unique_id + "_temperature";
  temp_conf["state_topic"] = topic_s.temperature;
  temp_conf["device_class"] = "temperature";
  temp_conf["unit_of_measurement"] = "°C";
  temp_conf["expire_after"] = "660";
  temp_conf["device"] = device_short;

  DynamicJsonDocument humidity_conf(2048);
  humidity_conf["name"] = user_s.device_name + " Humidity";
  humidity_conf["unique_id"] = factory_s.unique_id + "_humidity";
  humidity_conf["state_topic"] = topic_s.humidity;
  humidity_conf["device_class"] = "humidity";
  humidity_conf["unit_of_measurement"] = "%";
  humidity_conf["expire_after"] = "660";
  humidity_conf["device"] = device_short;

  DynamicJsonDocument battery_conf(2048);
  battery_conf["name"] = user_s.device_name + " Battery";
  battery_conf["unique_id"] = factory_s.unique_id + "_battery";
  battery_conf["state_topic"] = topic_s.battery;
  battery_conf["device_class"] = "battery";
  battery_conf["unit_of_measurement"] = "%";
  battery_conf["expire_after"] = "660";
  battery_conf["device"] = device_short;

  DynamicJsonDocument sensor_interval_conf(2048);
  sensor_interval_conf["name"] = user_s.device_name + " Sensor Interval";
  sensor_interval_conf["unique_id"] = factory_s.unique_id + "_sensor_interval";
  sensor_interval_conf["command_topic"] = topic_s.sensor_interval_cmd;
  sensor_interval_conf["state_topic"] = topic_s.sensor_interval_state;
  sensor_interval_conf["unit_of_measurement"] = "min";
  sensor_interval_conf["min"] = SEN_INTERVAL_MIN;
  sensor_interval_conf["max"] = SEN_INTERVAL_MAX;
  sensor_interval_conf["mode"] = "slider";
  sensor_interval_conf["icon"] = "mdi:timer-sand";
  sensor_interval_conf["retain"] = "true";
  sensor_interval_conf["device"] = device_short;

  DynamicJsonDocument *btn_label_configs[NUM_BUTTONS];
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    btn_label_configs[i] = new DynamicJsonDocument(2048);
    DynamicJsonDocument *conf = btn_label_configs[i];
    (*conf)["name"] =
        user_s.device_name + " Button " + String(i + 1) + " Label";
    (*conf)["unique_id"] =
        factory_s.unique_id + "_button_" + String(i + 1) + "_label";
    (*conf)["command_topic"] = topic_s.btn_label_cmd[i];
    (*conf)["state_topic"] = topic_s.btn_label_state[i];
    (*conf)["max"] = BTN_LABEL_MAXLEN;
    (*conf)["icon"] = String("mdi:numeric-") +  String(i + 1) + "-box";
    (*conf)["retain"] = "true";
    (*conf)["device"] = device_short;
    btn_label_configs[i] = conf;
  }

  // send mqtt msg
  size_t n;
  char buffer[2048];

  // button presses
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    n = serializeJson(*btn_press_configs[i], buffer);
    delete btn_press_configs[i];
    client.publish(button_press_config_topics[i].c_str(), buffer, true);
  }

  // temp
  n = serializeJson(temp_conf, buffer);
  client.publish(temperature_config_topic.c_str(), buffer, true);
  // humidity
  n = serializeJson(humidity_conf, buffer);
  client.publish(humidity_config_topic.c_str(), buffer, true);
  // battery
  n = serializeJson(battery_conf, buffer);
  client.publish(battery_config_topic.c_str(), buffer, true);
  // sensor interval
  n = serializeJson(sensor_interval_conf, buffer);
  client.publish(sensor_interval_config_topic.c_str(), buffer, true);

  // button labels
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    n = serializeJson(*btn_label_configs[i], buffer);
    delete btn_label_configs[i];
    client.publish(button_label_config_topics[i].c_str(), buffer, true);
  }
}
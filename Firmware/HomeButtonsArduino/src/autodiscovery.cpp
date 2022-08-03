#include "autodiscovery.h"
#include "network.h"

void send_autodiscovery_msg() {
    // Construct topics
    String trigger_topic_common = user_s.discovery_prefix + "/device_automation/" + factory_s.unique_id;
    String button_1_config_topic = trigger_topic_common + "/button1/config";
    String button_2_config_topic = trigger_topic_common + "/button2/config";
    String button_3_config_topic = trigger_topic_common + "/button3/config";
    String button_4_config_topic = trigger_topic_common + "/button4/config";
    String button_5_config_topic = trigger_topic_common + "/button5/config";
    String button_6_config_topic = trigger_topic_common + "/button6/config";
    String sensor_topic_common = user_s.discovery_prefix + "/sensor/" + factory_s.unique_id;
    String temperature_config_topic = sensor_topic_common + "/temperature/config";
    String humidity_config_topic = sensor_topic_common + "/humidity/config";
    String battery_config_topic = sensor_topic_common + "/battery/config";

    // Construct autodiscovery config json
    DynamicJsonDocument btn_1_conf(2048);
    btn_1_conf["automation_type"] = "trigger";
    btn_1_conf["topic"] = topic_s.button_1_press;
    btn_1_conf["payload"] = BTN_PRESS_PAYLOAD;
    btn_1_conf["type"] = "button_short_press";
    btn_1_conf["subtype"] = "button_1";
    JsonObject device1 = btn_1_conf.createNestedObject("device");
    device1["identifiers"][0] = factory_s.unique_id;
    device1["model"] = factory_s.model_name;
    device1["name"] = user_s.device_name;
    device1["sw_version"] = SW_VERSION;
    device1["manufacturer"] = MANUFACTURER;

    DynamicJsonDocument btn_2_conf(2048);
    btn_2_conf["automation_type"] = "trigger";
    btn_2_conf["topic"] = topic_s.button_2_press;
    btn_2_conf["payload"] = BTN_PRESS_PAYLOAD;
    btn_2_conf["type"] = "button_short_press";
    btn_2_conf["subtype"] = "button_2";
    JsonObject device2 = btn_2_conf.createNestedObject("device");
    device2["identifiers"][0] = factory_s.unique_id;

    DynamicJsonDocument btn_3_conf(2048);
    btn_3_conf["automation_type"] = "trigger";
    btn_3_conf["topic"] = topic_s.button_3_press;
    btn_3_conf["payload"] = BTN_PRESS_PAYLOAD;
    btn_3_conf["type"] = "button_short_press";
    btn_3_conf["subtype"] = "button_3";
    JsonObject device3 = btn_3_conf.createNestedObject("device");
    device3["identifiers"][0] = factory_s.unique_id;

    DynamicJsonDocument btn_4_conf(2048);
    btn_4_conf["automation_type"] = "trigger";
    btn_4_conf["topic"] = topic_s.button_4_press;
    btn_4_conf["payload"] = BTN_PRESS_PAYLOAD;
    btn_4_conf["type"] = "button_short_press";
    btn_4_conf["subtype"] = "button_4";
    JsonObject device4 = btn_4_conf.createNestedObject("device");
    device4["identifiers"][0] = factory_s.unique_id;

    DynamicJsonDocument btn_5_conf(2048);
    btn_5_conf["automation_type"] = "trigger";
    btn_5_conf["topic"] = topic_s.button_5_press;
    btn_5_conf["payload"] = BTN_PRESS_PAYLOAD;
    btn_5_conf["type"] = "button_short_press";
    btn_5_conf["subtype"] = "button_5";
    JsonObject device5 = btn_5_conf.createNestedObject("device");
    device5["identifiers"][0] = factory_s.unique_id;

    DynamicJsonDocument btn_6_conf(2048);
    btn_6_conf["automation_type"] = "trigger";
    btn_6_conf["topic"] = topic_s.button_6_press;
    btn_6_conf["payload"] = BTN_PRESS_PAYLOAD;
    btn_6_conf["type"] = "button_short_press";
    btn_6_conf["subtype"] = "button_6";
    JsonObject device6 = btn_6_conf.createNestedObject("device");
    device6["identifiers"][0] = factory_s.unique_id;

    DynamicJsonDocument temp_conf(2048);
    temp_conf["name"] = user_s.device_name + " Temperature";
    temp_conf["unique_id"] = factory_s.unique_id + "_temperature";
    temp_conf["state_topic"] = topic_s.temperature;
    temp_conf["device_class"] = "temperature";
    temp_conf["unit_of_measurement"] = "°C";
    temp_conf["expire_after"] = "660";
    JsonObject device7 = temp_conf.createNestedObject("device");
    device7["identifiers"][0] = factory_s.unique_id;

    DynamicJsonDocument humidity_conf(2048);
    humidity_conf["name"] = user_s.device_name + " Humidity";
    humidity_conf["unique_id"] = factory_s.unique_id + "_humidity";
    humidity_conf["state_topic"] = topic_s.humidity;
    humidity_conf["device_class"] = "humidity";
    humidity_conf["unit_of_measurement"] = "%";
    humidity_conf["expire_after"] = "660";
    JsonObject device8 = humidity_conf.createNestedObject("device");
    device8["identifiers"][0] = factory_s.unique_id;

    DynamicJsonDocument battery_conf(2048);
    battery_conf["name"] = user_s.device_name + " Battery";
    battery_conf["unique_id"] = factory_s.unique_id + "_battery";
    battery_conf["state_topic"] = topic_s.battery;
    battery_conf["device_class"] = "battery";
    battery_conf["unit_of_measurement"] = "%";
    battery_conf["expire_after"] = "660";
    JsonObject device9 = battery_conf.createNestedObject("device");
    device9["identifiers"][0] = factory_s.unique_id;

    // send mqtt msg
    size_t n;
    char buffer[2048];
    // 1
    n = serializeJson(btn_1_conf, buffer);
    client.publish(button_1_config_topic.c_str(), buffer, n);
    // 2
    n = serializeJson(btn_2_conf, buffer);
    client.publish(button_2_config_topic.c_str(), buffer, n);
    // 3
    n = serializeJson(btn_3_conf, buffer);
    client.publish(button_3_config_topic.c_str(), buffer, n);
    // 4
    n = serializeJson(btn_4_conf, buffer);
    client.publish(button_4_config_topic.c_str(), buffer, n);
    // 5
    n = serializeJson(btn_5_conf, buffer);
    client.publish(button_5_config_topic.c_str(), buffer, n);
    // 6
    n = serializeJson(btn_6_conf, buffer);
    client.publish(button_6_config_topic.c_str(), buffer, n);
    // temp
    n = serializeJson(temp_conf, buffer);
    client.publish(temperature_config_topic.c_str(), buffer, n);
    // humidity
    n = serializeJson(humidity_conf, buffer);
    client.publish(humidity_config_topic.c_str(), buffer, n);
    // battery
    n = serializeJson(battery_conf, buffer);
    client.publish(battery_config_topic.c_str(), buffer, n);
}
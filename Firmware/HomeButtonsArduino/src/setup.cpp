#include "setup.h"

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "config.h"
#include "display.h"
#include "hardware.h"
#include "state.h"

static WiFiManager wifi_manager;

static WiFiManagerParameter device_name_param("device_name", "Device Name", "",
                                              20);
static WiFiManagerParameter mqtt_server_param("mqtt_server", "MQTT Server", "",
                                              50);
static WiFiManagerParameter mqtt_port_param("mqtt_port", "MQTT Port", "", 6);
static WiFiManagerParameter mqtt_user_param("mqtt_user", "MQTT User", "", 50);
static WiFiManagerParameter mqtt_password_param("mqtt_password",
                                                "MQTT Password", "", 50);
static WiFiManagerParameter base_topic_param("base_topic", "Base Topic", "",
                                             50);
static WiFiManagerParameter discovery_prefix_param("disc_prefix",
                                                   "Discovery Prefix", "", 50);
static WiFiManagerParameter btn1_label_param("btn1_lbl", "Button 1 Label", "",
                                             BTN_LABEL_MAXLEN);
static WiFiManagerParameter btn2_label_param("btn2_lbl", "Button 2 Label", "",
                                             BTN_LABEL_MAXLEN);
static WiFiManagerParameter btn3_label_param("btn3_lbl", "Button 3 Label", "",
                                             BTN_LABEL_MAXLEN);
static WiFiManagerParameter btn4_label_param("btn4_lbl", "Button 4 Label", "",
                                             BTN_LABEL_MAXLEN);
static WiFiManagerParameter btn5_label_param("btn5_lbl", "Button 5 Label", "",
                                             BTN_LABEL_MAXLEN);
static WiFiManagerParameter btn6_label_param("btn6_lbl", "Button 6 Label", "",
                                             BTN_LABEL_MAXLEN);

static bool web_portal_saved = false;

void start_wifi_setup() {
  device_state.set_ap_ssid_and_password(String("HB-") + device_state.factory().random_id, SETUP_AP_PASSWORD);
  display.disp_ap_config();
  display.update();

  WiFi.mode(WIFI_STA);
  wifi_manager.setConfigPortalTimeout(SETUP_TIMEOUT);
  wifi_manager.resetSettings();
  wifi_manager.setTitle(WIFI_MANAGER_TITLE);
  wifi_manager.setBreakAfterConfig(true);
  wifi_manager.setDarkMode(true);
  wifi_manager.setShowInfoUpdate(false);

  bool wifi_connected = wifi_manager.startConfigPortal(
      device_state.network().ap_ssid.c_str(), device_state.network().ap_password.c_str());

  bool wifi_connected_2 = false;
  WiFi.mode(WIFI_STA);
  uint32_t wifi_start_time = millis();
  WiFi.begin();
  while (true) {
    delay(100);
    if (WiFi.status() == WL_CONNECTED) {
      wifi_connected_2 = true;
      break;
    } else if (millis() - wifi_start_time >= WIFI_TIMEOUT) {
      wifi_connected_2 = false;
      break;
    }
  }

  if (wifi_connected_2) {
    device_state.wifi_done = true;
    device_state.restart_to_setup = true;
    device_state.save_all();
    log_i("[W_SETUP] Wi-Fi connected.");
    display.disp_message_large("Wi-Fi\nconnected\n:)");
    display.update();
    delay(3000);
    ESP.restart();
  } else {
    device_state.wifi_done = false;
    device_state.save_all();
    log_w("[W_SETUP] Wi-Fi error.");
    display.disp_error("Wi-Fi\nconnection\nerror");
    display.update();
    delay(5000);
    ESP.restart();
  }
}

void save_params_callback() {
  device_state.set_device_name(device_name_param.getValue());
  device_state.set_mqtt_parameters(
    mqtt_server_param.getValue(),
    String(mqtt_port_param.getValue()).toInt(),
    mqtt_user_param.getValue(),
    mqtt_password_param.getValue(),
    base_topic_param.getValue(),
    discovery_prefix_param.getValue());
  device_state.set_btn_label(0, btn1_label_param.getValue());
  device_state.set_btn_label(1, btn2_label_param.getValue());
  device_state.set_btn_label(2, btn3_label_param.getValue());
  device_state.set_btn_label(3, btn4_label_param.getValue());
  device_state.set_btn_label(4, btn5_label_param.getValue());
  device_state.set_btn_label(5, btn6_label_param.getValue());
  web_portal_saved = true;
}

void start_setup() {
  // config
  wifi_manager.setTitle(WIFI_MANAGER_TITLE);
  wifi_manager.setSaveParamsCallback(save_params_callback);
  wifi_manager.setBreakAfterConfig(true);
  wifi_manager.setShowPassword(true);
  wifi_manager.setParamsPage(true);
  wifi_manager.setDarkMode(true);
  wifi_manager.setShowInfoUpdate(true);

  // parameters
  device_name_param.setValue(device_state.device_name().c_str(), 20);
  mqtt_server_param.setValue(device_state.network().mqtt.server.c_str(), 50);
  mqtt_port_param.setValue(String(device_state.network().mqtt.port).c_str(), 6);
  mqtt_user_param.setValue(device_state.network().mqtt.user.c_str(), 50);
  mqtt_password_param.setValue(device_state.network().mqtt.password.c_str(), 50);
  base_topic_param.setValue(device_state.network().mqtt.base_topic.c_str(), 50);
  discovery_prefix_param.setValue(device_state.network().mqtt.discovery_prefix.c_str(), 50);
  btn1_label_param.setValue(device_state.get_btn_label(0), 20);
  btn2_label_param.setValue(device_state.get_btn_label(1), 20);
  btn3_label_param.setValue(device_state.get_btn_label(2), 20);
  btn4_label_param.setValue(device_state.get_btn_label(3), 20);
  btn5_label_param.setValue(device_state.get_btn_label(4), 20);
  btn6_label_param.setValue(device_state.get_btn_label(5), 20);
  wifi_manager.addParameter(&device_name_param);
  wifi_manager.addParameter(&mqtt_server_param);
  wifi_manager.addParameter(&mqtt_port_param);
  wifi_manager.addParameter(&mqtt_user_param);
  wifi_manager.addParameter(&mqtt_password_param);
  wifi_manager.addParameter(&base_topic_param);
  wifi_manager.addParameter(&discovery_prefix_param);
  wifi_manager.addParameter(&btn1_label_param);
  wifi_manager.addParameter(&btn2_label_param);
  wifi_manager.addParameter(&btn3_label_param);
  wifi_manager.addParameter(&btn4_label_param);
  wifi_manager.addParameter(&btn5_label_param);
  wifi_manager.addParameter(&btn6_label_param);

  // connect Wi-Fi
  WiFi.mode(WIFI_STA);
  uint32_t wifi_start_time = millis();
  WiFi.begin();
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    if (millis() - wifi_start_time >= WIFI_TIMEOUT) {
      device_state.wifi_done = false;
      device_state.save_all();
      log_w("[SETUP] Wi-Fi error.");
      display.disp_error("Wi-Fi\nerror");
      display.update();
      delay(3000);
      ESP.restart();
    }
  }
  device_state.set_ip(WiFi.localIP().toString());
  display.disp_web_config();
  display.update();
  uint32_t setup_start_time = millis();

  web_portal_saved = false;
  wifi_manager.startWebPortal();
  while (millis() - setup_start_time < SETUP_TIMEOUT * 1000L) {
    wifi_manager.process();
    if (HW.digitalReadAny() || web_portal_saved) {
      break;
    }
    delay(10);
  }
  wifi_manager.stopWebPortal();

  // test MQTT connection
  uint32_t mqtt_start_time = millis();
  WiFiClient wifi_client;
  PubSubClient mqtt_client(wifi_client);
  mqtt_client.setServer(device_state.network().mqtt.server.c_str(), device_state.network().mqtt.port);
  if (device_state.network().mqtt.user.length() > 0 &&
      device_state.network().mqtt.password.length() > 0) {
    mqtt_client.connect(device_state.factory().unique_id.c_str(),
                               device_state.network().mqtt.user.c_str(),
                               device_state.network().mqtt.password.c_str());
  } else {
    mqtt_client.connect(device_state.factory().unique_id.c_str());
  }

  display.disp_message("Confirming\nsetup...");
  display.update();

  while (!mqtt_client.connected()) {
    delay(10);
    if (millis() - mqtt_start_time >= MQTT_TIMEOUT) {
      device_state.setup_done = false;
      device_state.save_all();
      log_w("[SETUP] MQTT error.");
      display.disp_error("MQTT\nerror");
      display.update();
      delay(3000);
      ESP.restart();
    }
  }

  mqtt_client.disconnect();
  WiFi.disconnect(true);
  device_state.setup_done = true;
  device_state.send_discovery_config = true;
  device_state.save_all();

  log_i("[SETUP] setup successful");
  display.disp_message_large("Setup\ncomplete\n:)");
  display.update();
  delay(3000);
  ESP.restart();
  
}
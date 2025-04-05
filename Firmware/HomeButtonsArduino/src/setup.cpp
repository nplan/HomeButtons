#include "setup.h"
#include "app.h"

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>

#include "utils.h"

static WiFiManager wifi_manager;

static WiFiManagerParameter device_name_param("device_name", "Device Name", "",
                                              20);
static WiFiManagerParameter mqtt_server_param("mqtt_server", "MQTT Server", "",
                                              32);
static WiFiManagerParameter mqtt_port_param("mqtt_port", "MQTT Port", "", 6);
static WiFiManagerParameter mqtt_user_param("mqtt_user", "MQTT User", "", 64);
static WiFiManagerParameter mqtt_password_param("mqtt_password",
                                                "MQTT Password", "", 64);
static WiFiManagerParameter base_topic_param("base_topic", "Base Topic", "",
                                             64);
static WiFiManagerParameter discovery_prefix_param("disc_prefix",
                                                   "Discovery Prefix", "", 64);
static WiFiManagerParameter static_ip_param("static_ip", "Static IP", "", 15);
static WiFiManagerParameter gateway_param("gateway", "Gateway", "", 15);
static WiFiManagerParameter subnet_param("subnet", "Subnet Mask", "", 15);
static WiFiManagerParameter dns_param("dns", "Primary DNS Server", "", 15);
static WiFiManagerParameter dns2_param("dns2", "Secondary DNS Server", "", 15);

#if defined(HAS_TH_SENSOR)
static WiFiManagerParameter temp_unit_param("temp_unit", "Temperature Unit", "",
                                            1);
#endif

#if defined(HOME_BUTTONS_INDUSTRIAL)
static WiFiManagerParameter button_config_param("btn_conf", "Button Config", "",
                                                NUM_BUTTONS - 1);
#endif

#if defined(HAS_DISPLAY)
static char* button_ids[NUM_BUTTONS];
static char* button_labels[NUM_BUTTONS];
static WiFiManagerParameter* btn_label_params[NUM_BUTTONS];
static WiFiManagerParameter icon_server_param("icon_srv", "Icon Server", "",
                                              128);

void allocate_btn_label_params() {
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    button_ids[i] = new char[11];
    button_labels[i] = new char[17];
    snprintf(button_ids[i], 11, "btn%d_lbl", i + 1);
    snprintf(button_labels[i], 17, "Button %d Label", i + 1);

    btn_label_params[i] = new WiFiManagerParameter(
        button_ids[i], button_labels[i], "", BTN_LABEL_MAXLEN);
  }
}

void set_btn_label_params_from_device_state(DeviceState& device_state_) {
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    btn_label_params[i]->setValue(device_state_.get_btn_label(i + 1).c_str(),
                                  BTN_LABEL_MAXLEN);
  }
}

void set_device_state_from_btn_label_params(DeviceState& device_state_) {
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    device_state_.set_btn_label(i + 1, btn_label_params[i]->getValue());
  }
}
#endif

void HBSetup::start_wifi_setup() {
  info("Wi-Fi setup");
  app_.bsl_input_.PauseSwitchModeAll();
#if defined(HAS_DISPLAY)
  app_.display_.disp_ap_config();
#else
  app_.bsl_input_.LEDPulse(2, LED_DFLT_BRIGHT, 2000);
#endif

#if defined(HOME_BUTTONS_DEBUG)
  Serial.begin(115200);
  wifi_manager.setDebugOutput(true, WM_DEBUG_DEV);
#endif

  WiFi.mode(WIFI_STA);
  wifi_manager.setTitle(app_.device_state_.get_model_name_w_rand_id().c_str());
  wifi_manager.setBreakAfterConfig(true);
  wifi_manager.setDarkMode(true);
  wifi_manager.setShowInfoUpdate(false);
  wifi_manager.setConfigPortalBlocking(false);

  uint32_t setup_start_time = millis();
  wifi_manager.startConfigPortal(app_.device_state_.get_ap_ssid().c_str(),
                                 app_.device_state_.get_ap_password());
  bool user_stopped = false;
  while (true) {
    bool connected = wifi_manager.process();
    if (millis() - setup_start_time > SETUP_TIMEOUT * 1000L) {
      debug("Wi-Fi config portal stopped, timeout");
      break;
    }
    if (connected) {
      debug("Wi-Fi config portal stopped, connected");
      break;
    }
    if (app_.hw_.any_button_pressed()) {
      user_stopped = true;
      debug("Wi-Fi config portal stopped by user");
      break;
    }
    delay(250);
  }

  if (user_stopped) {
    info("Wi-Fi config portal stopped by user");
#if defined(HAS_DISPLAY)
    app_.display_.disp_error("Wi-Fi\nsetup\ncancelled");
    delay(5000);
#else
    app_.bsl_input_.LEDBlink(2, 5, LED_DFLT_BRIGHT, 200, 160, false);
    delay(3000);
#endif
    ESP.restart();
  }

  info("Wi-Fi config portal stopped, trying to connect to Wi-Fi...");

  bool wifi_connected = false;
  WiFi.mode(WIFI_STA);
  uint32_t wifi_start_time = millis();
  WiFi.begin();
  while (true) {
    delay(100);
    if (WiFi.status() == WL_CONNECTED) {
      wifi_connected = true;
      break;
    } else if (millis() - wifi_start_time >= WIFI_TIMEOUT) {
      wifi_connected = false;
      break;
    }
  }

  if (wifi_connected) {
    SSIDType ssid(WiFi.SSID());
    if (!(ssid == app_.device_state_.user_preferences().network.ssid)) {
      app_.device_state_.clear_static_ip_config();
    }
    app_.device_state_.persisted().wifi_done = true;
    app_.device_state_.persisted().restart_to_setup = true;
    app_.device_state_.persisted().silent_restart = true;
    app_.device_state_.save_all();
    info("Wi-Fi connected :)");
#if defined(HAS_DISPLAY)
    app_.display_.disp_message_large("Wi-Fi\nconnected\n:)");
    delay(3000);
#else
    app_.bsl_input_.LEDBlink(2, 2, LED_DFLT_BRIGHT, 500, 400, false);
    delay(3000);
#endif
    ESP.restart();
  } else {
    app_.device_state_.persisted().wifi_done = false;
    app_.device_state_.persisted().silent_restart = true;
    app_.device_state_.save_all();
    warning("Wi-Fi error :(");
#if defined(HAS_DISPLAY)
    app_.display_.disp_error("Wi-Fi\nconnection\nerror");
    delay(5000);
#else
    app_.bsl_input_.LEDBlink(2, 5, LED_DFLT_BRIGHT, 200, 160, false);
    delay(3000);
#endif
    ESP.restart();
  }
}

void HBSetup::save_params_callback() {
  app_.device_state_.set_device_name(DeviceName{device_name_param.getValue()});
  app_.device_state_.set_mqtt_parameters(
      mqtt_server_param.getValue(), String(mqtt_port_param.getValue()).toInt(),
      mqtt_user_param.getValue(), mqtt_password_param.getValue(),
      base_topic_param.getValue(), discovery_prefix_param.getValue());

#if defined(HAS_DISPLAY)
  set_device_state_from_btn_label_params(app_.device_state_);
  app_.device_state_.set_icon_server(IconServerType{
      ensure_trailing_slash(IconServerType{icon_server_param.getValue()})});
#endif

#if defined(HAS_TH_SENSOR)
  app_.device_state_.set_temp_unit(StaticString<1>(temp_unit_param.getValue()));
#endif

#if defined(HOME_BUTTONS_INDUSTRIAL)
  app_.device_state_.set_btn_conf_string(
      BtnConfString{button_config_param.getValue()});
#endif

  SSIDType ssid(WiFi.SSID());
  IPAddress static_ip, gateway, subnet, dns, dns2;
  static_ip.fromString(static_ip_param.getValue());
  gateway.fromString(gateway_param.getValue());
  subnet.fromString(subnet_param.getValue());
  dns.fromString(dns_param.getValue());
  dns2.fromString(dns2_param.getValue());
  app_.device_state_.set_static_ip_config(ssid, static_ip, gateway, subnet, dns,
                                          dns2);
  web_portal_saved_ = true;
}

void HBSetup::start_setup() {
  info("Setup");
  app_.bsl_input_.PauseSwitchModeAll();
  // config
  wifi_manager.setTitle(app_.device_state_.get_model_name_w_rand_id().c_str());
  wifi_manager.setSaveParamsCallback(
      std::bind(&HBSetup::save_params_callback, this));
  wifi_manager.setBreakAfterConfig(true);
  wifi_manager.setShowPassword(true);
  wifi_manager.setParamsPage(true);
  wifi_manager.setDarkMode(true);
  wifi_manager.setShowInfoUpdate(true);

  // hostname
  wifi_manager.setHostname(app_.device_state_.get_hostname().c_str());

  // parameters
  device_name_param.setValue(app_.device_state_.device_name().c_str(), 20);
  mqtt_server_param.setValue(
      app_.device_state_.user_preferences().mqtt.server.c_str(), 32);
  mqtt_port_param.setValue(
      String(app_.device_state_.user_preferences().mqtt.port).c_str(), 6);
  mqtt_user_param.setValue(
      app_.device_state_.user_preferences().mqtt.user.c_str(), 64);
  mqtt_password_param.setValue(
      app_.device_state_.user_preferences().mqtt.password.c_str(), 64);
  base_topic_param.setValue(
      app_.device_state_.user_preferences().mqtt.base_topic.c_str(), 64);
  discovery_prefix_param.setValue(
      app_.device_state_.user_preferences().mqtt.discovery_prefix.c_str(), 64);
  static_ip_param.setValue(app_.device_state_.user_preferences()
                               .network.static_ip.toString()
                               .c_str(),
                           15);
  gateway_param.setValue(
      app_.device_state_.user_preferences().network.gateway.toString().c_str(),
      15);
  subnet_param.setValue(
      app_.device_state_.user_preferences().network.subnet.toString().c_str(),
      15);
  dns_param.setValue(
      app_.device_state_.user_preferences().network.dns.toString().c_str(), 15);
  dns2_param.setValue(
      app_.device_state_.user_preferences().network.dns2.toString().c_str(),
      15);

#if defined(HAS_DISPLAY)
  allocate_btn_label_params();
  set_btn_label_params_from_device_state(app_.device_state_);
  icon_server_param.setValue(
      app_.device_state_.user_preferences().icon_server.c_str(), 128);
#endif

#if defined(HAS_TH_SENSOR)
  temp_unit_param.setValue(app_.device_state_.get_temp_unit().c_str(), 1);
#endif

#if defined(HOME_BUTTONS_INDUSTRIAL)
  button_config_param.setValue(
      app_.device_state_.user_preferences().btn_conf_string.c_str(),
      NUM_BUTTONS - 1);
#endif

  wifi_manager.addParameter(&device_name_param);
  wifi_manager.addParameter(&mqtt_server_param);
  wifi_manager.addParameter(&mqtt_port_param);
  wifi_manager.addParameter(&mqtt_user_param);
  wifi_manager.addParameter(&mqtt_password_param);
  wifi_manager.addParameter(&base_topic_param);
  wifi_manager.addParameter(&discovery_prefix_param);
  wifi_manager.addParameter(&static_ip_param);
  wifi_manager.addParameter(&gateway_param);
  wifi_manager.addParameter(&subnet_param);
  wifi_manager.addParameter(&dns_param);
  wifi_manager.addParameter(&dns2_param);

#if defined(HAS_DISPLAY)
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    wifi_manager.addParameter(btn_label_params[i]);
  }
  wifi_manager.addParameter(&icon_server_param);
#endif

#if defined(HAS_TH_SENSOR)
  wifi_manager.addParameter(&temp_unit_param);
#endif

#if defined(HOME_BUTTONS_INDUSTRIAL)
  wifi_manager.addParameter(&button_config_param);
#endif

#if defined(HAS_DISPLAY)
  app_.display_.disp_message("Entering\nSETUP...");
#else
  app_.bsl_input_.LEDPulse(1, LED_DFLT_BRIGHT, 500);
  delay(2000);
#endif

  // set static IP if configured
  StaticIPConfig static_ip_config =
      validate_static_ip_config(app_.device_state_.user_preferences().network);
  if (static_ip_config.valid) {
    WiFi.config(static_ip_config.static_ip, static_ip_config.gateway,
                static_ip_config.subnet, static_ip_config.dns,
                static_ip_config.dns2);
    info("Using static IP %s, Gateway %s, Subnet %s, DNS1 %s, DNS2 %s",
         ip_address_to_static_string(static_ip_config.static_ip).c_str(),
         ip_address_to_static_string(static_ip_config.gateway).c_str(),
         ip_address_to_static_string(static_ip_config.subnet).c_str(),
         ip_address_to_static_string(static_ip_config.dns).c_str(),
         ip_address_to_static_string(static_ip_config.dns2).c_str());
  } else {
    info("Using DHCP. Static IP not set or not valid.");
  }

  // connect Wi-Fi
  WiFi.mode(WIFI_STA);
  int remaining_tries = MAX_WIFI_RETRIES_DURING_MQTT_SETUP;

  while (true) {
    uint32_t wifi_start_time = millis();
    WiFi.begin();
    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      if (millis() - wifi_start_time >= WIFI_TIMEOUT) {
        break;
      }
    }

    if (WiFi.status() == WL_CONNECTED) {
      break;
    } else if (remaining_tries > 0) {
      remaining_tries--;
      warning("Wi-Fi error, retrying (remaining tries: %d)", remaining_tries);
      WiFi.disconnect();
      delay(1000);
    } else {
      app_.device_state_.persisted().silent_restart = true;
      app_.device_state_.save_all();
      warning("Wi-Fi error.");
#if defined(HAS_DISPLAY)
      app_.display_.disp_error("Wi-Fi\nerror");
      delay(3000);
#else
      app_.bsl_input_.LEDBlink(1, 5, LED_DFLT_BRIGHT, 200, 160, false);
      delay(3000);
#endif
      ESP.restart();
    }
  }

  // mDNS
  MDNS.begin(app_.device_state_.get_hostname().c_str());

  app_.device_state_.set_ip(WiFi.localIP());
  info("Connect to http://%s or http://%s.local",
       ip_address_to_static_string(WiFi.localIP()).c_str(),
       app_.device_state_.get_hostname().c_str());
#if defined(HAS_DISPLAY)
  app_.display_.disp_web_config();
#else
  app_.bsl_input_.LEDPulse(1, LED_DFLT_BRIGHT, 2000);
#endif
  uint32_t setup_start_time = millis();

  web_portal_saved_ = false;
  wifi_manager.startWebPortal();
  while (millis() - setup_start_time < SETUP_TIMEOUT * 1000L) {
    wifi_manager.process();
    if (app_.hw_.any_button_pressed() || web_portal_saved_) {
      break;
    }
    delay(10);
  }
  wifi_manager.stopWebPortal();

  if (!web_portal_saved_) {
    debug("User triggered exit setup");
    app_.device_state_.persisted().silent_restart = true;
    app_.device_state_.save_all();
    ESP.restart();
  }

  // test MQTT connection
  uint32_t mqtt_start_time = millis();
  WiFiClient wifi_client;
  PubSubClient mqtt_client(wifi_client);
  debug("Trying to connect to mqtt://%s:%d",
        app_.device_state_.user_preferences().mqtt.server.c_str(),
        app_.device_state_.user_preferences().mqtt.port);
  mqtt_client.setServer(
      app_.device_state_.user_preferences().mqtt.server.c_str(),
      app_.device_state_.user_preferences().mqtt.port);
  if (app_.device_state_.user_preferences().mqtt.user.length() > 0 &&
      app_.device_state_.user_preferences().mqtt.password.length() > 0) {
    mqtt_client.connect(
        app_.device_state_.factory().unique_id.c_str(),
        app_.device_state_.user_preferences().mqtt.user.c_str(),
        app_.device_state_.user_preferences().mqtt.password.c_str());
  } else {
    mqtt_client.connect(app_.device_state_.factory().unique_id.c_str());
  }

#if defined(HAS_DISPLAY)
  app_.display_.disp_message("Confirming\nsetup...");
#else
  app_.bsl_input_.LEDPulse(1, LED_DFLT_BRIGHT, 250);
#endif

  while (!mqtt_client.connected()) {
    delay(10);
    if (millis() - mqtt_start_time >= MQTT_TIMEOUT) {
      app_.device_state_.persisted().setup_done = false;
      app_.device_state_.persisted().silent_restart = true;
      app_.device_state_.save_all();
      warning("MQTT error.");
#if defined(HAS_DISPLAY)
      app_.display_.disp_error("MQTT\nerror");
      delay(3000);
#else
      app_.bsl_input_.LEDBlink(1, 5, LED_DFLT_BRIGHT, 200, 160, false);
      delay(3000);
#endif
      ESP.restart();
    }
  }

  mqtt_client.disconnect();
  WiFi.disconnect(true);
  app_.device_state_.persisted().setup_done = true;
  app_.device_state_.persisted().silent_restart = true;
  app_.device_state_.persisted().connect_on_restart = true;
  app_.device_state_.save_all();

  info("setup successful");
#if defined(HAS_DISPLAY)
  app_.display_.disp_message_large("Setup\ncomplete\n:)");
  delay(3000);
#else
  app_.bsl_input_.LEDBlink(1, 2, LED_DFLT_BRIGHT, 500, 400, false);
  delay(3000);
#endif
  ESP.restart();
}

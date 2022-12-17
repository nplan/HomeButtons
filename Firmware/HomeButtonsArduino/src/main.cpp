#include <Arduino.h>
#include <WiFiManager.h>

#include "autodiscovery.h"
#include "config.h"
#include "display.h"
#include "factory.h"
#include "hardware.h"
#include "hw_tests.h"
#include "network.h"
#include "prefs.h"
#include "utils.h"

// ------ global variables ------
WiFiManager wifi_manager;
uint32_t info_screen_start_time = 0;

bool web_portal_saved = false;

WiFiManagerParameter device_name_param("device_name", "Device Name", "", 20);
WiFiManagerParameter mqtt_server_param("mqtt_server", "MQTT Server", "", 50);
WiFiManagerParameter mqtt_port_param("mqtt_port", "MQTT Port", "", 6);
WiFiManagerParameter mqtt_user_param("mqtt_user", "MQTT User", "", 50);
WiFiManagerParameter mqtt_password_param("mqtt_password", "MQTT Password", "",
                                         50);
WiFiManagerParameter base_topic_param("base_topic", "Base Topic", "", 50);
WiFiManagerParameter discovery_prefix_param("disc_prefix", "Discovery Prefix",
                                            "", 50);
WiFiManagerParameter btn1_label_param("btn1_lbl", "Button 1 Label", "", BTN_LABEL_MAXLEN);
WiFiManagerParameter btn2_label_param("btn2_lbl", "Button 2 Label", "", BTN_LABEL_MAXLEN);
WiFiManagerParameter btn3_label_param("btn3_lbl", "Button 3 Label", "", BTN_LABEL_MAXLEN);
WiFiManagerParameter btn4_label_param("btn4_lbl", "Button 4 Label", "", BTN_LABEL_MAXLEN);
WiFiManagerParameter btn5_label_param("btn5_lbl", "Button 5 Label", "", BTN_LABEL_MAXLEN);
WiFiManagerParameter btn6_label_param("btn6_lbl", "Button 6 Label", "", BTN_LABEL_MAXLEN);

enum BootReason {
  NO_REASON,
  TMR,
  RST,
  BTN,
  BTN_MEDIUM,
  BTN_LONG,
  BTN_EXTRA_LONG,
  BTN_ULTRA_LONG
};
BootReason boot_reason = NO_REASON;
enum ControlFlow {
  NO_FLOW,
  WIFI_SETUP,
  SETUP,
  RESET,
  BTN_PRESS,
  FAC_RESET,
  TIMER,
  SHOW_INFO,
  AFTER_UPDATE
};
ControlFlow control_flow = NO_FLOW;
uint32_t btn_press_time = 0;
uint32_t config_start_time = 0;
enum WakeupButton { NO_BTN, BTN1, BTN2, BTN3, BTN4, BTN5, BTN6 };
WakeupButton wakeup_button = NO_BTN;
int16_t wakeup_pin = -1;

void set_topics() {
  String topic_common = user_s.base_topic + "/" + user_s.device_name + "/";
  String topic_cmd = topic_common + "cmd/";

  // button press topics
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    topic_s.btn_press[i] = topic_common + "button_" + String(i + 1);
  }

  // sensors
  topic_s.temperature = topic_common + "temperature";
  topic_s.humidity = topic_common + "humidity";
  topic_s.battery = topic_common + "battery";

  // sensor interval
  topic_s.sensor_interval_state = topic_common + "sensor_interval";
  topic_s.sensor_interval_cmd = topic_cmd + "sensor_interval";

  // button label state & cmd
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    topic_s.btn_label_state[i] =
        topic_common + "btn_" + String(i + 1) + "_label";
    topic_s.btn_label_cmd[i] = topic_cmd + "btn_" + String(i + 1) + "_label";
  }
}

void save_params_clbk() {
  user_s.device_name = device_name_param.getValue();
  user_s.mqtt_server = mqtt_server_param.getValue();
  user_s.mqtt_port = String(mqtt_port_param.getValue()).toInt();
  user_s.mqtt_user = mqtt_user_param.getValue();
  user_s.mqtt_password = mqtt_password_param.getValue();
  user_s.base_topic = base_topic_param.getValue();
  user_s.discovery_prefix = discovery_prefix_param.getValue();
  user_s.btn_1_label = check_button_label(btn1_label_param.getValue());
  user_s.btn_2_label = check_button_label(btn2_label_param.getValue());
  user_s.btn_3_label = check_button_label(btn3_label_param.getValue());
  user_s.btn_4_label = check_button_label(btn4_label_param.getValue());
  user_s.btn_5_label = check_button_label(btn5_label_param.getValue());
  user_s.btn_6_label = check_button_label(btn6_label_param.getValue());
  save_user_settings(user_s);
  web_portal_saved = true;
}

void display_buttons() {
  eink::display_buttons_UTF8(user_s.btn_1_label, user_s.btn_2_label,
                             user_s.btn_3_label, user_s.btn_4_label,
                             user_s.btn_5_label, user_s.btn_6_label);
}

void start_esp_sleep() {
  esp_sleep_enable_ext1_wakeup(HW.WAKE_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
  if (persisted_s.wifi_done && persisted_s.setup_done &&
      !persisted_s.low_batt_mode && !persisted_s.check_connection) {
    if (persisted_s.info_screen_showing) {
      esp_sleep_enable_timer_wakeup(INFO_SCREEN_DISP_TIME * 1000UL);
    } else {
      esp_sleep_enable_timer_wakeup(user_s.sensor_interval * 60000000UL);
    }
  }
  log_i("starting deep sleep");
  esp_deep_sleep_start();
}

void go_to_sleep() {
  eink::hibernate();
  set_all_leds(0);
  start_esp_sleep();
}

void setup() {
  log_i("woke up");

  // ------ read preferences ------
  factory_s = read_factory_settings();
  user_s = read_user_settings();
  persisted_s = read_persisted_vars();
  set_topics();

  // ------ factory mode ------
  if (factory_s.serial_number.length() < 1) {
    log_i("first boot, starting factory mode...");
    if (factory_mode()) {
      factory_s = read_factory_settings();
      log_i("factory settings complete. going to sleep");
      eink::display_welcome_screen(factory_s.unique_id.c_str(), factory_s);
      go_to_sleep();
    }
  }

  // ------ init hardware ------
  log_i("hardware version: %s", factory_s.hw_version);
  init_hardware(factory_s.hw_version);
  eink::begin();  // must be before ledAttachPin (reserves GPIO37 = SPIDQS)
  begin_hardware();

  // ------ check battery voltage first thing ------
  float batt_volt = read_battery_voltage();
  uint8_t batt_pct = batt_volt2percent(batt_volt);
  if (persisted_s.low_batt_mode) {
    if (batt_volt > HW.BATT_HISTERESIS_VOLT) {
      persisted_s.low_batt_mode = false;
      log_i("low batt mode disabled");
      display_buttons();
    } else {
      go_to_sleep();
    }
  } else {
    if (batt_volt < HW.MIN_BATT_VOLT) {
      // delay and check again
      delay(1000);
      batt_volt = read_battery_voltage();
      if (batt_volt < HW.MIN_BATT_VOLT) {
        eink::display_turned_off_please_recharge_screen();
        persisted_s.low_batt_mode = true;
        log_w("low batt mode enabled. voltage: %f", batt_volt);
        save_persisted_vars(persisted_s);
        go_to_sleep();
      }
    }
  }

  // ------ read temp & humidity ------
  float temperature_meas;
  float humidity_meas;
  read_temp_hmd(temperature_meas, humidity_meas);

  // ------ boot reason ------
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1) {
    btn_press_time = millis();
    uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
    wakeup_pin = (log(GPIO_reason)) / log(2);
    log_i("wakeup pin: %d", wakeup_pin);
    int16_t ctrl = 0;
    bool break_loop = false;
    while (!break_loop) {
      switch (ctrl) {
        case 0:
          if (!digitalRead(wakeup_pin)) {
            boot_reason = BTN;
            ctrl = -1;
          } else if (millis() - btn_press_time > MEDIUM_PRESS_TIME) {
            eink::display_info_screen(temperature_meas, humidity_meas,
                                      batt_volt2percent(batt_volt));
            ctrl = 1;
          }
          break;
        case 1:
          if (!digitalRead(wakeup_pin)) {
            delay(100);  // debounce
            boot_reason = BTN_MEDIUM;
            ctrl = -1;
          } else if (millis() - btn_press_time > LONG_PRESS_TIME) {
            eink::display_string(
                "Release\nfor\nSETUP\n\nKeep holding\nfor\nWi-Fi SETUP");
            ctrl = 3;
          }
          break;
        case 3:
          if (!digitalRead(wakeup_pin)) {
            boot_reason = BTN_LONG;
            ctrl = -1;
          } else if (millis() - btn_press_time > EXTRA_LONG_PRESS_TIME) {
            ctrl = 4;
            eink::display_string(
                "Release\nfor\nWi-Fi SETUP\n\nKeep "
                "holding\nfor\nFACTORY\nRESET");
          }
          break;
        case 4:
          if (!digitalRead(wakeup_pin)) {
            boot_reason = BTN_EXTRA_LONG;
            ctrl = -1;
          } else if (millis() - btn_press_time > ULTRA_LONG_PRESS_TIME) {
            boot_reason = BTN_ULTRA_LONG;
            ctrl = -1;
          }
          break;
        case -1:
          ctrl = 0;
          break_loop = true;
          break;
      }
    }
  } else if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
    boot_reason = TMR;
  } else {
    boot_reason = RST;
  }

  switch (boot_reason) {
    case NO_REASON:
      log_i("boot reason: no reason");
      control_flow = NO_FLOW;
      break;
    case RST:
      log_i("boot reason: reset");
      if (persisted_s.reset_to_setup) {
        control_flow = SETUP;
        persisted_s.reset_to_setup = false;
        save_persisted_vars(persisted_s);
      } else if (persisted_s.last_sw_ver != SW_VERSION) {
        control_flow = AFTER_UPDATE;
      } else {
        control_flow = RESET;
      }
      break;
    case TMR:
      log_i("boot reason: timer");
      if (!persisted_s.wifi_done && !persisted_s.setup_done)
        control_flow = NO_FLOW;
      else
        control_flow = TIMER;
      break;
    case BTN:
      log_i("boot reason: btn");
      if (!persisted_s.wifi_done)
        control_flow = WIFI_SETUP;
      else if (!persisted_s.setup_done)
        control_flow = SETUP;
      else
        control_flow = BTN_PRESS;
      break;
    case BTN_MEDIUM:
      log_i("boot reason: btn med");
      if (!persisted_s.wifi_done)
        control_flow = WIFI_SETUP;
      else if (!persisted_s.setup_done)
        control_flow = SETUP;
      else
        control_flow = SHOW_INFO;
      break;
    case BTN_LONG:
      log_i("boot reason: btn long");
      if (!persisted_s.wifi_done)
        control_flow = WIFI_SETUP;
      else
        control_flow = SETUP;
      break;
    case BTN_EXTRA_LONG:
      log_i("boot reason: btn extra long");
      control_flow = WIFI_SETUP;
      break;
    case BTN_ULTRA_LONG:
      log_i("boot_reason: btn ultra long");
      control_flow = FAC_RESET;
      break;
  }

  // ------ main logic switch ------
  switch (control_flow) {
    case WIFI_SETUP: {
      log_i("control flow: wifi setup");
      persisted_s.info_screen_showing = false;
      persisted_s.charge_complete_showing = false;
      persisted_s.check_connection = false;
      delay(50);  // debounce
      eink::display_string("Starting\nWi-Fi setup...");
      persisted_s.wifi_quick_connect = false;
      config_start_time = millis();
      // start wifi manager
      String ap_name = String("HB-") + factory_s.random_id;
      eink::display_ap_config_screen(ap_name, AP_PASSWORD);

      WiFi.mode(WIFI_STA);
      wifi_manager.setConfigPortalTimeout(CONFIG_TIMEOUT);
      wifi_manager.resetSettings();
      wifi_manager.setTitle(WIFI_MANAGER_TITLE);
      wifi_manager.setBreakAfterConfig(true);
      wifi_manager.setDarkMode(true);
      wifi_manager.setShowInfoUpdate(false);

      bool wifi_connected =
          wifi_manager.startConfigPortal(ap_name.c_str(), AP_PASSWORD);
      bool wifi_connected_2 = connect_wifi();

      if (!wifi_connected &&
          !wifi_connected_2) {  // double check that wifi can not be connected
                                // with connect_wifi() function
        persisted_s.wifi_done = false;
        eink::display_error("Wi-Fi\nerror");
        delay(5000);
        eink::display_welcome_screen(factory_s.unique_id.c_str(), factory_s);
        break;
      }

      persisted_s.wifi_done = true;
      persisted_s.reset_to_setup = true;
      save_persisted_vars(persisted_s);  // save now because restarting before
                                         // end of setup function
      eink::display_wifi_connected_screen();
      delay(3000);
      ESP.restart();
      break;
    }
    case SETUP: {
      log_i("control flow: setup");
      persisted_s.info_screen_showing = false;
      persisted_s.charge_complete_showing = false;
      persisted_s.check_connection = false;
      delay(50);  // debounce
      eink::display_string("Starting\nsetup...");
      bool wifi_connected = connect_wifi();
      if (!wifi_connected) {
        eink::display_error("Wi-Fi\nerror");
        delay(3000);
        if (persisted_s.setup_done) {
          display_buttons();
        } else {
          eink::display_welcome_screen(factory_s.unique_id.c_str(), factory_s);
        }
        break;
      }
      persisted_s.wifi_quick_connect =
          false;  // maybe user changed wifi settings
      eink::display_web_config_screen(WiFi.localIP().toString());
      config_start_time = millis();

      wifi_manager.setTitle(WIFI_MANAGER_TITLE);
      wifi_manager.setSaveParamsCallback(save_params_clbk);
      wifi_manager.setBreakAfterConfig(true);
      wifi_manager.setShowPassword(true);
      wifi_manager.setParamsPage(true);
      wifi_manager.setDarkMode(true);
      wifi_manager.setShowInfoUpdate(true);

      // Parameters
      device_name_param.setValue(user_s.device_name.c_str(), 20);
      mqtt_server_param.setValue(user_s.mqtt_server.c_str(), 50);
      mqtt_port_param.setValue(String(user_s.mqtt_port).c_str(), 6);
      mqtt_user_param.setValue(user_s.mqtt_user.c_str(), 50);
      mqtt_password_param.setValue(user_s.mqtt_password.c_str(), 50);
      base_topic_param.setValue(user_s.base_topic.c_str(), 50);
      discovery_prefix_param.setValue(user_s.discovery_prefix.c_str(), 50);
      btn1_label_param.setValue(user_s.btn_1_label.c_str(), 20);
      btn2_label_param.setValue(user_s.btn_2_label.c_str(), 20);
      btn3_label_param.setValue(user_s.btn_3_label.c_str(), 20);
      btn4_label_param.setValue(user_s.btn_4_label.c_str(), 20);
      btn5_label_param.setValue(user_s.btn_5_label.c_str(), 20);
      btn6_label_param.setValue(user_s.btn_6_label.c_str(), 20);
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

      web_portal_saved =
          false;  // set to true in web portal save button callback
      wifi_manager.startWebPortal();
      while (millis() - config_start_time < CONFIG_TIMEOUT * 1000L) {
        wifi_manager.process();
        if (digitalReadAny() || web_portal_saved) {
          break;
        }
      }
      wifi_manager.stopWebPortal();
      eink::display_string("Exiting setup...");

      bool mqtt_connected = connect_mqtt();
      if (!mqtt_connected) {
        persisted_s.setup_done = false;
        eink::display_error("MQTT\nerror");
        delay(3000);
        eink::display_welcome_screen(factory_s.unique_id.c_str(), factory_s);
        break;
      }
      set_topics();
      send_autodiscovery_msg();

      client.publish(topic_s.temperature.c_str(),
                     String(temperature_meas).c_str());
      client.publish(topic_s.humidity.c_str(), String(humidity_meas).c_str());
      client.publish(topic_s.battery.c_str(), String(batt_pct).c_str());
      client.publish(topic_s.sensor_interval_state.c_str(),
                     String(user_s.sensor_interval).c_str());

      for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
        client.publish(topic_s.btn_label_state[i].c_str(),
                       user_s.get_btn_label(i).c_str(), true);
      }

      persisted_s.setup_done = true;
      eink::display_setup_complete_screen();
      delay(3000);
      display_buttons();
      break;
    }
    case BTN_PRESS: {
      log_i("control flow: btn press");

      if (persisted_s.info_screen_showing) {
        persisted_s.info_screen_showing = false;
        display_buttons();
        break;
      } else if (persisted_s.charge_complete_showing) {
        persisted_s.charge_complete_showing = false;
        display_buttons();
        break;
      } else if (persisted_s.check_connection) {
        persisted_s.check_connection = false;
        persisted_s.failed_connections = 0;
        display_buttons();
        break;
      }

      if (wakeup_pin == HW.BTN1_PIN) {
        wakeup_button = BTN1;
        set_led(HW.LED1_CH, HW.LED_BRIGHT_DFLT);
      } else if (wakeup_pin == HW.BTN2_PIN) {
        wakeup_button = BTN3;
        set_led(HW.LED2_CH, HW.LED_BRIGHT_DFLT);
      } else if (wakeup_pin == HW.BTN3_PIN) {
        wakeup_button = BTN5;
        set_led(HW.LED3_CH, HW.LED_BRIGHT_DFLT);
      } else if (wakeup_pin == HW.BTN4_PIN) {
        wakeup_button = BTN6;
        set_led(HW.LED4_CH, HW.LED_BRIGHT_DFLT);
      } else if (wakeup_pin == HW.BTN5_PIN) {
        wakeup_button = BTN4;
        set_led(HW.LED5_CH, HW.LED_BRIGHT_DFLT);
      } else if (wakeup_pin == HW.BTN6_PIN) {
        wakeup_button = BTN2;
        set_led(HW.LED6_CH, HW.LED_BRIGHT_DFLT);
      }

      bool wifi_connected = connect_wifi();
      if (!wifi_connected) {
        eink::display_error("Wi-Fi\nerror");
        delay(3000);
        display_buttons();
        break;
      }
      bool mqtt_connected = connect_mqtt();
      if (!mqtt_connected) {
        eink::display_error("MQTT\nerror");
        delay(3000);
        display_buttons();
        break;
      }
      switch (wakeup_button) {
        case BTN1:
          client.publish(topic_s.btn_press[0].c_str(), BTN_PRESS_PAYLOAD);
          break;
        case BTN2:
          client.publish(topic_s.btn_press[1].c_str(), BTN_PRESS_PAYLOAD);
          break;
        case BTN3:
          client.publish(topic_s.btn_press[2].c_str(), BTN_PRESS_PAYLOAD);
          break;
        case BTN4:
          client.publish(topic_s.btn_press[3].c_str(), BTN_PRESS_PAYLOAD);
          break;
        case BTN5:
          client.publish(topic_s.btn_press[4].c_str(), BTN_PRESS_PAYLOAD);
          break;
        case BTN6:
          client.publish(topic_s.btn_press[5].c_str(), BTN_PRESS_PAYLOAD);
          break;
      }
      client.publish(topic_s.temperature.c_str(),
                     String(temperature_meas).c_str());
      client.publish(topic_s.humidity.c_str(), String(humidity_meas).c_str());
      client.publish(topic_s.battery.c_str(), String(batt_pct).c_str());
      if (batt_volt < HW.WARN_BATT_VOLT) {
        eink::display_please_recharge_soon_screen();
        delay(2000);
        display_buttons();
      }
      break;
    }
    case SHOW_INFO: {
      log_i("control flow: show info");
      persisted_s.info_screen_showing = true;
      break;
    }
    case RESET: {
      log_i("control flow: reset");
      eink::display_string("Device RESET");
      persisted_s.info_screen_showing = false;
      persisted_s.charge_complete_showing = false;
      persisted_s.check_connection = false;
      if (persisted_s.wifi_done && persisted_s.setup_done)
        display_buttons();
      else
        eink::display_welcome_screen(factory_s.unique_id.c_str(), factory_s);
      break;
    }
    case AFTER_UPDATE: {
      log_i("control flow: after update");
      eink::display_string(String("Firmware\nupdated to\n") + SW_VERSION);
      persisted_s.last_sw_ver = SW_VERSION;
      if (persisted_s.wifi_done && persisted_s.setup_done) {
        bool wifi_connected = connect_wifi();
        if (!wifi_connected) {
          eink::display_error("Wi-Fi\nerror");
          delay(3000);
          display_buttons();
        }
        bool mqtt_connected = connect_mqtt();
        if (!mqtt_connected) {
          eink::display_error("MQTT\nerror");
          delay(3000);
          display_buttons();
        }
        if (wifi_connected && mqtt_connected) {
          send_autodiscovery_msg();
          // publish retained states
          client.publish(topic_s.sensor_interval_state.c_str(),
                         String(user_s.sensor_interval).c_str(), true);
          for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
            client.publish(topic_s.btn_label_state[i].c_str(),
                           user_s.get_btn_label(i).c_str(), true);
          }
          disconnect_mqtt();
        }
      }
      save_persisted_vars(persisted_s);
      ESP.restart();
      break;
    }
    case FAC_RESET: {
      log_i("control flow: fac reset");
      eink::display_string("Starting\nFactory\nRESET...");
      clear_all_preferences();
      wifi_manager.resetSettings();
      delay(3000);
      eink::display_string("Factory\nRESET\ncomplete");
      ESP.restart();
      break;
    }
    case TIMER: {
      log_i("control flow: timer");
      if (persisted_s.info_screen_showing) {
        persisted_s.info_screen_showing = false;
        display_buttons();
        break;
      }

      if (is_charger_in_standby() && !persisted_s.charge_complete_showing) {
        persisted_s.charge_complete_showing = true;
        eink::display_fully_charged_screen();
      }

      if (persisted_s.check_connection) {
        break;
      }

      bool wifi_connected = connect_wifi();
      if (!wifi_connected) {
        persisted_s.failed_connections ++;
        if (persisted_s.failed_connections >= MAX_FAILED_CONNECTIONS) {
          persisted_s.check_connection = true;
          eink::display_check_connection_screen();
        }
        break;
      }
      bool mqtt_connected = connect_mqtt();
      if (!mqtt_connected) {
        persisted_s.failed_connections ++;
        if (persisted_s.failed_connections >= MAX_FAILED_CONNECTIONS) {
          persisted_s.check_connection = true;
          eink::display_check_connection_screen();
        }
        break;
      }
      client.publish(topic_s.temperature.c_str(),
                     String(temperature_meas).c_str());
      client.publish(topic_s.humidity.c_str(), String(humidity_meas).c_str());
      client.publish(topic_s.battery.c_str(), String(batt_pct).c_str());
      break;
    }
    case NO_FLOW: {
      // do nothing
      break;
    }
    default: {
      break;
    }
  }

  disconnect_mqtt();
  save_persisted_vars(persisted_s);
  save_user_settings(user_s);
  if (flags_s.buttons_redraw) {
    flags_s.buttons_redraw = false;
    display_buttons();
  }
  go_to_sleep();
}

void loop() {
  // will never get here :)
}

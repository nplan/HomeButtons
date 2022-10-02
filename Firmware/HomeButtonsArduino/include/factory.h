#ifndef FACTORY_H
#define FACTORY_H

#include "display.h"
#include "hardware.h"
#include "prefs.h"
#include "network.h"

void test_leds() {
  set_all_leds(255);
  delay(250);
  set_all_leds(0);
  delay(250);
}

void test_buttons() {
  set_led(HW.LED1_CH, 255);
  while (!digitalRead(HW.BTN1_PIN)) {
  }
  set_led(HW.LED1_CH, 0);

  set_led(HW.LED2_CH, 255);
  while (!digitalRead(HW.BTN2_PIN)) {
  }
  set_led(HW.LED2_CH, 0);

  set_led(HW.LED3_CH, 255);
  while (!digitalRead(HW.BTN3_PIN)) {
  }
  set_led(HW.LED3_CH, 0);

  set_led(HW.LED4_CH, 255);
  while (!digitalRead(HW.BTN4_PIN)) {
  }
  set_led(HW.LED4_CH, 0);

  set_led(HW.LED5_CH, 255);
  while (!digitalRead(HW.BTN5_PIN)) {
  }
  set_led(HW.LED5_CH, 0);

  set_led(HW.LED6_CH, 255);
  while (!digitalRead(HW.BTN6_PIN)) {
  }
  set_led(HW.LED6_CH, 0);
}

void test_display() {
  eink::display_test_screen();
  while (!digitalReadAny()) {
  }
  eink::display_black_screen();
  while (!digitalReadAny()) {
  }
  eink::display_white_screen();
  while (!digitalReadAny()) {
  }
}

void test_wifi(NetworkSettings settings, FactorySettings factory) {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);

  log_i("Connecting to WiFi...");
  WiFi.begin(settings.wifi_ssid.c_str(), settings.wifi_password.c_str());
  while (true) {
    delay(100);
    if (WiFi.status() == WL_CONNECTED) {
      break;
    }
  }
  log_i("WiFi connected. IP: %S", WiFi.localIP().toString());
  log_i("Connecting to MQTT...");
  client.setServer(settings.mqtt_server.c_str(), settings.mqtt_port);
  String client_id = "HBTNS-" + factory.serial_number + "-factory";
  while (!client.connected()) {
    client.connect(client_id.c_str(), settings.mqtt_user.c_str(), settings.mqtt_password.c_str());
    delay(100);
  }
  log_i("MWTT connected");
  client.publish("homebuttons-factory/test", "OK");
  log_i("MQTT payload 'OK' sent to topic: homebuttons-factory/test");
  disconnect_mqtt();
  log_i("MQTT disconnected");
  WiFi.disconnect(true, true);
  log_i("WiFi disconnected");
}

bool run_tests() {
  if (digitalReadAny()) {
    log_e("button pressed at start of test");
    return false;
  }
  while (!digitalReadAny()) {
    test_leds();
  }
  test_buttons();
  test_display();
  return true;
}

void sendOK() { Serial.println("OK"); }

void sendFAIL() { Serial.println("FAIL"); }

bool factory_mode() {
  NetworkSettings networkSettings = {};

  Serial.begin(115200);
  delay(1000);

  FactorySettings settings = {};

  log_i("factory mode active");

  while (1) {
    Serial.setTimeout(5000);
    String msg = Serial.readStringUntil('\r');

    if (msg.length() > 0) {
      String cmd = msg.substring(0, 2);
      String pld = msg.substring(3);
      log_i("received cmd: %s, pld: %s", cmd.c_str(), pld.c_str());

      if (cmd == "ST") {
        if (settings.hw_version.length() > 0) {
          init_hardware(settings.hw_version);
          eink::begin();
          begin_hardware();
          log_i("starting hw test...");
          if (run_tests()) {
            log_i("test complete");
            sendOK();
          } else {
            sendFAIL();
          }
        } else {
          log_w("set hardware version before running tests");
        }
      } else if (cmd == "SN") {
        if (pld.length() == 8) {
          settings.serial_number = pld;
          log_i("setting serial number to: %s", settings.serial_number.c_str());
          sendOK();
        } else {
          log_w("incorrect serial number: %s", pld.c_str());
          sendFAIL();
        }
      } else if (cmd == "RI") {
        if (pld.length() == 6) {
          settings.random_id = pld;
          log_i("setting random id to: %s", settings.random_id.c_str());
          sendOK();
        } else {
          log_w("incorrect random id: %s", pld.c_str());
          sendFAIL();
        }
      } else if (cmd == "MN") {
        if (pld.length() > 0 && pld.length() <= 20) {
          settings.model_name = pld;
          log_i("setting model name to: %s", settings.model_name.c_str());
          sendOK();
        } else {
          log_w("incorrect model name: %s", pld.c_str());
          sendFAIL();
        }
      } else if (cmd == "MI") {
        if (pld.length() == 2) {
          settings.model_id = pld;
          log_i("setting model id to: %s", settings.model_id);
          sendOK();
        } else {
          log_w("incorrect model id: %s", pld.c_str());
          sendFAIL();
        }
      } else if (cmd == "HV") {
        if (pld.length() == 3) {
          settings.hw_version = pld;
          log_i("setting hw version to: %s", settings.hw_version);
          sendOK();
        } else {
          log_w("incorrect hw version: %s", pld.c_str());
          sendFAIL();
        }
      } else if (cmd == "WS") {
        if (pld.length() > 0) {
          networkSettings.wifi_ssid = pld;
          log_i("wifi ssid: %s", pld.c_str());
          sendOK();
        } else {
          log_w("incorrect wifi ssid");
          sendFAIL();
        }
        sendOK();
      } else if (cmd == "WP") {
        if (pld.length() > 0) {
          networkSettings.wifi_password = pld;
          log_i("wifi password: %s", pld.c_str());
          sendOK();
        } else {
          log_w("incorrect wifi password");
          sendFAIL();
        }
      } else if (cmd == "MS") {
        if (pld.length() > 0) {
          networkSettings.mqtt_server = pld;
          log_i("mqtt server: %s", pld.c_str());
          sendOK();
        } else {
          log_w("incorrect mqtt server");
          sendFAIL();
        }
      } else if (cmd == "MU") {
        if (pld.length() > 0) {
          networkSettings.mqtt_user = pld;
          log_i("mqtt user: %s", pld.c_str());
          sendOK();
        } else {
          log_w("incorrect mqtt user");
          sendFAIL();
        }
      } else if (cmd == "MP") {
        if (pld.length() > 0) {
          networkSettings.mqtt_password = pld;
          log_i("mqtt password: %s", pld.c_str());
          sendOK();
        } else {
          log_w("incorrect mqtt password");
          sendFAIL();
        }
      } else if (cmd == "MT") {
        if (pld.length() > 0) {
          networkSettings.mqtt_port = pld.toInt();
          log_i("mqtt port: %d", networkSettings.mqtt_port);
          sendOK();
        } else {
          log_w("incorrect mqtt port");
          sendFAIL();
        }
      } else if (cmd == "TW") {
        if (settings.serial_number.length() > 0) {
          log_i("starting wifi test...");
          test_wifi(networkSettings, settings);
          sendOK();
        }
        else {
          log_w("set serial number before wifi test");
          sendFAIL();
        }

      } else if (cmd == "OK") {
        if (settings.serial_number.length() > 0 &&
            settings.random_id.length() > 0 &&
            settings.model_name.length() > 0 &&
            settings.model_id.length() > 0 &&
            settings.hw_version.length() > 0) {
          settings.unique_id = String("HBTNS-") + settings.serial_number + "-" +
                               settings.random_id;

          log_i("settings confirmed. saving to memory");
          log_i(
              "settings:\n\
                           serial number: %s\n\
                           random id: %s\n\
                           device model name: %s\n\
                           device model id: %s\n\
                           hw version: %s\n\
                           unique_id: %s\n",
              settings.serial_number.c_str(), settings.random_id.c_str(),
              settings.model_name.c_str(), settings.model_id,
              settings.hw_version, settings.unique_id);
          sendOK();
          save_factory_settings(settings);
          return true;
        } else {
          log_w("settings missing or incorrect");
          sendFAIL();
        }
      }
    }
  }

  // ------ test LEDs ------
  while (1) {
    test_buttons();
  }

  return false;
}

#endif
#ifndef FACTORY_H
#define FACTORY_H

#include "display.h"
#include "hardware.h"
#include "prefs.h"

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
  Serial.begin(115200);
  delay(1000);

  FactorySettings settings = {};

  log_i("factory mode active");

  while (1) {
    Serial.setTimeout(5000);
    String msg = Serial.readStringUntil('\r');

    if (msg.length() > 0) {
      char cmd = msg[0];
      String pld = msg.substring(2);
      // log_i("received cmd: %c, pld: %s", cmd, pld.c_str());

      if (cmd == 'T') {
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
          log_w("set hardware version befor running tests");
        }
      } else if (cmd == 'S') {
        if (pld.length() == 8) {
          settings.serial_number = pld;
          log_i("setting serial number to: %s", settings.serial_number.c_str());
          sendOK();
        } else {
          log_w("incorrect serial number: %s", pld.c_str());
          sendFAIL();
        }
      } else if (cmd == 'I') {
        if (pld.length() == 6) {
          settings.random_id = pld;
          log_i("setting random id to: %s", settings.random_id.c_str());
          sendOK();
        } else {
          log_w("incorrect random id: %s", pld.c_str());
          sendFAIL();
        }
      } else if (cmd == 'M') {
        if (pld.length() > 0 && pld.length() <= 20) {
          settings.model_name = pld;
          log_i("setting model name to: %s", settings.model_name.c_str());
          sendOK();
        } else {
          log_w("incorrect model name: %s", pld.c_str());
          sendFAIL();
        }
      } else if (cmd == 'D') {
        if (pld.length() == 2) {
          settings.model_id = pld;
          log_i("setting model id to: %s", settings.model_id);
          sendOK();
        } else {
          log_w("incorrect model id: %s", pld.c_str());
          sendFAIL();
        }
      } else if (cmd == 'V') {
        if (pld.length() == 3) {
          settings.hw_version = pld;
          log_i("setting hw version to: %s", settings.hw_version);
          sendOK();
        } else {
          log_w("incorrect hw version: %s", pld.c_str());
          sendFAIL();
        }
      } else if (cmd == 'E') {
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
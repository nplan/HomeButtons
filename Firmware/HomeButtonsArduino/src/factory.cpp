#include "factory.h"

namespace factory {

struct TestSettings {
  float target_temp;
  float target_humidity;
};

struct NetworkSettings {
  String wifi_ssid = "";
  String wifi_password = "";
  String mqtt_server = "";
  String mqtt_user = "";
  String mqtt_password = "";
  int32_t mqtt_port = 0;
};

void test_leds() {
  HW.set_all_leds(255);
  delay(250);
  HW.set_all_leds(0);
  delay(250);
}

void test_buttons() {
  HW.set_led(HW.LED1_CH, 255);
  while (!digitalRead(HW.BTN1_PIN)) {
  }
  HW.set_led(HW.LED1_CH, 0);

  HW.set_led(HW.LED2_CH, 255);
  while (!digitalRead(HW.BTN2_PIN)) {
  }
  HW.set_led(HW.LED2_CH, 0);

  HW.set_led(HW.LED3_CH, 255);
  while (!digitalRead(HW.BTN3_PIN)) {
  }
  HW.set_led(HW.LED3_CH, 0);

  HW.set_led(HW.LED4_CH, 255);
  while (!digitalRead(HW.BTN4_PIN)) {
  }
  HW.set_led(HW.LED4_CH, 0);

  HW.set_led(HW.LED5_CH, 255);
  while (!digitalRead(HW.BTN5_PIN)) {
  }
  HW.set_led(HW.LED5_CH, 0);

  HW.set_led(HW.LED6_CH, 255);
  while (!digitalRead(HW.BTN6_PIN)) {
  }
  HW.set_led(HW.LED6_CH, 0);
}

void test_display(Display& display) {
  display.disp_test(false);
  display.update();
  while (!HW.digitalReadAny()) {
  }
  display.disp_test(true);
  display.update();
  while (!HW.digitalReadAny()) {
  }
}

void test_wifi(const NetworkSettings& settings,
               const DeviceState& device_state) {
  WiFiClient wifi_client;
  PubSubClient mqtt_client(wifi_client);

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
  mqtt_client.setServer(settings.mqtt_server.c_str(), settings.mqtt_port);
  String client_id =
      "HBTNS-" + device_state.factory().serial_number + "-factory";
  while (!mqtt_client.connected()) {
    mqtt_client.connect(client_id.c_str(), settings.mqtt_user.c_str(),
                        settings.mqtt_password.c_str());
    delay(100);
  }
  log_i("MWTT connected");
  mqtt_client.publish("homebuttons-factory/test", "OK");
  log_i("MQTT payload 'OK' sent to topic: homebuttons-factory/test");
  // disconnect
  unsigned long tm = millis();
  while (millis() - tm < MQTT_DISCONNECT_TIMEOUT) {
    mqtt_client.loop();
  }
  // disconnect and wait until closed
  mqtt_client.disconnect();
  wifi_client.flush();
  // wait until connection is closed completely
  while (mqtt_client.state() != -1) {
    mqtt_client.loop();
    delay(10);
  }
  log_i("MQTT disconnected");
  WiFi.disconnect(true, true);
  log_i("WiFi disconnected");
}

bool test_sensors(const TestSettings& settings) {
  if (settings.target_temp == 0. && settings.target_humidity == 0.) {
    log_e("target values not set");
    return false;
  }

  float temp_val;
  float hmd_val;
  HW.read_temp_hmd(temp_val, hmd_val);

  if (temp_val <= settings.target_temp - 5.0 ||
      temp_val >= settings.target_temp + 5.0) {
    log_i("Temp test fail. Measured: %f deg C.", temp_val);
    return false;
  }
  if (hmd_val <= settings.target_humidity - 10.0 ||
      temp_val >= settings.target_humidity + 10.0) {
    log_i("Humidity test fail. Measured: %f %.", hmd_val);
    return false;
  }
  return true;
}

bool run_tests(const TestSettings& settings, Display& display) {
  if (HW.digitalReadAny()) {
    log_e("button pressed at start of test");
    return false;
  }
  while (!HW.digitalReadAny()) {
    test_leds();
  }
  test_buttons();
  if (!test_sensors(settings)) {
    return false;
  }
  test_display(display);
  return true;
}

void sendOK() { Serial.println("OK"); }

void sendFAIL() { Serial.println("FAIL"); }

void factory_mode(DeviceState& device_state, Display& display) {
  NetworkSettings networkSettings = {};
  TestSettings test_settings = {};

  Serial.begin(115200);
  delay(1000);

  log_i("factory mode active");

  while (1) {
    Serial.setTimeout(5000);
    String msg = Serial.readStringUntil('\r');

    if (msg.length() > 0) {
      String cmd = msg.substring(0, 2);
      String pld = msg.substring(3);
      log_i("received cmd: %s, pld: %s", cmd.c_str(), pld.c_str());

      if (cmd == "ST") {
        if (device_state.factory().hw_version.length() > 0) {
          HW.init(device_state.factory().hw_version);
          display.begin();
          HW.begin();
          log_i("starting hw test...");
          if (run_tests(test_settings, display)) {
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
          device_state.set_serial_number(pld);
          log_i("setting serial number to: %s",
                device_state.factory().serial_number.c_str());
          sendOK();
        } else {
          log_w("incorrect serial number: %s", pld.c_str());
          sendFAIL();
        }
      } else if (cmd == "RI") {
        if (pld.length() == 6) {
          device_state.set_random_id(pld);
          log_i("setting random id to: %s",
                device_state.factory().random_id.c_str());
          sendOK();
        } else {
          log_w("incorrect random id: %s", pld.c_str());
          sendFAIL();
        }
      } else if (cmd == "MN") {
        if (pld.length() > 0 && pld.length() <= 20) {
          device_state.set_model_name(pld);
          log_i("setting model name to: %s",
                device_state.factory().model_name.c_str());
          sendOK();
        } else {
          log_w("incorrect model name: %s", pld.c_str());
          sendFAIL();
        }
      } else if (cmd == "MI") {
        if (pld.length() == 2) {
          device_state.set_model_id(pld);
          log_i("setting model id to: %s", device_state.factory().model_id);
          sendOK();
        } else {
          log_w("incorrect model id: %s", pld.c_str());
          sendFAIL();
        }
      } else if (cmd == "HV") {
        if (pld.length() == 3) {
          device_state.set_hw_version(pld);
          log_i("setting hw version to: %s", device_state.factory().hw_version);
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
        if (device_state.factory().serial_number.length() > 0) {
          log_i("starting wifi test...");
          test_wifi(networkSettings, device_state);
          sendOK();
        } else {
          log_w("set serial number before wifi test");
          sendFAIL();
        }
      } else if (cmd == "TT") {
        if (pld.length() > 0) {
          test_settings.target_temp = pld.toFloat();
          log_i("target temp set to: %f C", test_settings.target_temp);
          sendOK();
        } else {
          log_w("incorrect target temp");
          sendFAIL();
        }
      } else if (cmd == "TH") {
        if (pld.length() > 0) {
          test_settings.target_humidity = pld.toFloat();
          log_i("target humidity set to: %f C", test_settings.target_humidity);
          sendOK();
        } else {
          log_w("incorrect target humidity");
          sendFAIL();
        }
      } else if (cmd == "OK") {
        if (device_state.factory().serial_number.length() > 0 &&
            device_state.factory().random_id.length() > 0 &&
            device_state.factory().model_name.length() > 0 &&
            device_state.factory().model_id.length() > 0 &&
            device_state.factory().hw_version.length() > 0) {
          device_state.set_unique_id(String("HBTNS-") +
                                     device_state.factory().serial_number +
                                     "-" + device_state.factory().random_id);

          log_i("settings confirmed. saving to memory");
          log_i(
              "settings:\n\
                           serial number: %s\n\
                           random id: %s\n\
                           device model name: %s\n\
                           device model id: %s\n\
                           hw version: %s\n\
                           unique_id: %s\n",
              device_state.factory().serial_number.c_str(),
              device_state.factory().random_id.c_str(),
              device_state.factory().model_name.c_str(),
              device_state.factory().model_id.c_str(),
              device_state.factory().hw_version.c_str(),
              device_state.factory().unique_id.c_str());
          sendOK();
          device_state.save_factory();
          display.end();
          display.update();
          return;
        } else {
          log_w("settings missing or incorrect");
          sendFAIL();
        }
      }
    }
  }
}

} /* namespace factory */
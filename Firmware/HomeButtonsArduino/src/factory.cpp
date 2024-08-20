#include "factory.h"

#include "config.h"
#include "static_string.h"

#include <Preferences.h>
#include <ArduinoJson.h>

#include <PubSubClient.h>
#include <WiFi.h>

static constexpr char FAC_TEST_BASE_TOPIC[] = "homebuttons-factory/devices";
static constexpr uint16_t TEST_ICON_SIZE = 100;
static constexpr uint32_t BUTTON_TEST_TIMEOUT = 60000L;
static constexpr std::array<const char*, 8> TEST_SPEC_KEYS = {
    "temp_ref",       "temp_tol",       "humd_ref", "humd_tol",
    "batt_mvolt_ref", "batt_mvolt_tol", "mdi_name", "disp_text"};

void FactoryTest::_mqtt_callback(const char* topic, uint8_t* payload,
                                 uint32_t length) {
  StaticJsonDocument<512> doc;
  DeserializationError json_error = deserializeJson(doc, payload);

  if (json_error != DeserializationError::Ok) {
    error("Failed to parse JSON: %s", json_error.c_str());
    return;
  }

  if (!doc.containsKey("parameters")) {
    error("Missing parameters");
    return;
  } else {
    for (auto k : TEST_SPEC_KEYS) {
      if (!doc["parameters"].containsKey(k)) {
        error("Missing parameter %s", k);
        return;
      }
    }
  }

  JsonObject parameters = doc["parameters"].as<JsonObject>();
  test_spec_.temp_ref = parameters["temp_ref"].as<float>();
  test_spec_.temp_tol = parameters["temp_tol"].as<float>();
  test_spec_.humd_ref = parameters["humd_ref"].as<float>();
  test_spec_.humd_tol = parameters["humd_tol"].as<float>();
  test_spec_.batt_mvolt_ref = parameters["batt_mvolt_ref"].as<unsigned int>();
  test_spec_.batt_mvolt_tol = parameters["batt_mvolt_tol"].as<unsigned int>();
  test_spec_.mdi_name = parameters["mdi_name"].as<const char*>();
  test_spec_.disp_text = parameters["disp_text"].as<const char*>();

  test_spec_.received = true;
  info("Test spec received");
}

bool FactoryTest::is_test_required() {
  Preferences prefs;

  prefs.begin("fac_test", true);
  bool do_test = prefs.getBool("do_test", false);
  prefs.end();
  return do_test;
}

bool FactoryTest::run_test() {
  Preferences prefs;
  FacTestParams params = {};

  bool passed = true;

  prefs.begin("fac_test", true);
  params.do_test = prefs.getBool("do_test", false);
  params.wifi_ssid = prefs.getString("wifi_ssid", "");
  params.wifi_password = prefs.getString("wifi_pass", "");
  params.mqtt_server.fromString(prefs.getString("mqtt_srv", ""));
  params.mqtt_port = prefs.getUInt("mqtt_port", 0);
  params.mqtt_user = prefs.getString("mqtt_user", "");
  params.mqtt_password = prefs.getString("mqtt_pass", "");
  prefs.end();

  info("Starting factory test...");
  app_._begin_hw();

#if defined(HAS_DISPLAY)
  // format SPIFFS if needed
  if (!SPIFFS.begin()) {
    info("Formatting icon storage...");
    app_.display_.disp_message("Formatting\nIcon\nStorage...", 0);
    app_.display_.update();
    SPIFFS.format();
  } else {
    SPIFFS.end();
    debug("SPIFFS test mount OK");
  }
  app_.display_.disp_message_large("FACTORY");
  app_.display_.update();
#endif

  WiFiClient wifi_client;
  PubSubClient mqtt_client(wifi_client);
  mqtt_client.setCallback(
      std::bind(&FactoryTest::_mqtt_callback, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3));
  mqtt_client.setBufferSize(1024);

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);

  info("Connecting to WiFi...");
  WiFi.begin(params.wifi_ssid.c_str(), params.wifi_password.c_str());
  while (true) {
    delay(100);
    if (WiFi.status() == WL_CONNECTED) {
      break;
    }
  }
  info("WiFi connected. IP: %s", WiFi.localIP().toString().c_str());
  info("Connecting to MQTT...");
  info("mqtt_server %s, port %d", params.mqtt_server.toString().c_str(),
       params.mqtt_port);
  mqtt_client.setServer(params.mqtt_server, params.mqtt_port);
  while (!mqtt_client.connected()) {
    mqtt_client.connect(app_.hw_.get_unique_id(), params.mqtt_user.c_str(),
                        params.mqtt_password.c_str());
    delay(100);
  }
  info("MQTT connected");

  StaticString<256> test_topic("%s/%s/test_start", FAC_TEST_BASE_TOPIC,
                               app_.hw_.get_serial_number());
  mqtt_client.subscribe(test_topic.c_str());

  // send device discovery message
  StaticJsonDocument<256> device_doc;
  device_doc["serial"] = app_.hw_.get_serial_number();
  device_doc["random_id"] = app_.hw_.get_random_id();
  device_doc["model_id"] = app_.hw_.get_model_id();
  device_doc["fw_version"] = SW_VERSION;
  device_doc["app_.hw__version"] = app_.hw_.get_hw_version();

  char buffer[512];
  serializeJson(device_doc, buffer, sizeof(buffer));

  StaticString<256> topic("%s/%s", FAC_TEST_BASE_TOPIC,
                          app_.hw_.get_serial_number());
  mqtt_client.publish(topic.c_str(), buffer);

  // wait for test start message
  while (!test_spec_.received) {
    mqtt_client.loop();
    delay(10);
  }

#if defined(HAS_DISPLAY)
  // test display
  info("Testing display");
  bool display_passed = true;
  MDIHelper mdi;
  mdi.begin();

  mdi.remove(test_spec_.mdi_name.c_str(), TEST_ICON_SIZE);

  if (mdi.check_connection()) {
    if (!mdi.download(test_spec_.mdi_name.c_str(), TEST_ICON_SIZE)) {
      error("MDI download failed");
      display_passed = false;
    }
  } else {
    error("MDI server connection failed");
    display_passed = false;
  }
  mdi.end();

  app_.display_.disp_test(test_spec_.disp_text.c_str(),
                          test_spec_.mdi_name.c_str(), 100);
  app_.display_.update();
  passed = passed && display_passed;
#endif

#if defined(HAS_BUTTON_UI)
  // test LEDs & buttons
  info("Testing LEDs & buttons");
  bool button_passed = true;
  app_.hw_.set_all_leds(255);
  bool pressed[NUM_BUTTONS] = {};
  uint32_t start_time = millis();
  while (true) {
    if (millis() - start_time > BUTTON_TEST_TIMEOUT) {
      error("button test timeout");
      button_passed = false;
      break;
    }
    for (int i = 0; i < NUM_BUTTONS; i++) {
      if (app_.hw_.button_pressed(i + 1)) {
        pressed[i] = true;
        app_.hw_.set_led_num(i + 1, 0);
      }
    }
    bool all_pressed = true;
    for (int i = 0; i < NUM_BUTTONS; i++) {
      all_pressed = pressed[i];
      if (!all_pressed) {
        break;
      }
    }
    if (all_pressed) {
      break;
    }
  }
  passed = passed && button_passed;
#endif

#if defined(HAS_TH_SENSOR)
  // test sensors
  info("Testing sensors");
  bool sensor_passed = true;
  float temp_val;
  float hmd_val;
  app_.hw_.read_temp_hmd(temp_val, hmd_val);

  if (temp_val <= test_spec_.temp_ref - test_spec_.temp_tol ||
      temp_val >= test_spec_.temp_ref + test_spec_.temp_tol) {
    sensor_passed = false;
    error("temp test fail. Measured: %f, expected: %f +/- %f", temp_val,
          test_spec_.temp_ref, test_spec_.temp_tol);
  }
  if (hmd_val <= test_spec_.humd_ref - test_spec_.humd_tol ||
      hmd_val >= test_spec_.humd_ref + test_spec_.humd_tol) {
    sensor_passed = false;
    error("humidity test fail. Measured: %f, expected: %f +/- %f", hmd_val,
          test_spec_.humd_ref, test_spec_.humd_tol);
  }
#if defined(HAS_BATTERY)
  uint16_t batt_v = app_.hw_.read_battery_voltage() * 1000.;
  if (batt_v <= test_spec_.batt_mvolt_ref - test_spec_.batt_mvolt_tol ||
      batt_v >= test_spec_.batt_mvolt_ref + test_spec_.batt_mvolt_tol) {
    sensor_passed = false;
    error("battery test fail. Measured: %d mV, expected: %d +/- %d mV", batt_v,
          test_spec_.batt_mvolt_ref, test_spec_.batt_mvolt_tol);
  }
#endif
  passed = passed && sensor_passed;
#endif

  // send test results
  StaticJsonDocument<1024> result_doc;

  JsonObject device = result_doc.createNestedObject("device");
  device["serial"] = app_.hw_.get_serial_number();
  device["random_id"] = app_.hw_.get_random_id();
  device["model_id"] = app_.hw_.get_model_id();
  device["fw_version"] = SW_VERSION;
  device["app_.hw_version"] = app_.hw_.get_hw_version();
  result_doc["passed"] = passed;

#if defined(HAS_TH_SENSOR) || defined(HAS_BATTERY)
  JsonObject parameters = result_doc.createNestedObject("parameters");
#endif
#if defined(HAS_TH_SENSOR)
  parameters["measured_temp"] = temp_val;
  parameters["measured_humidity"] = hmd_val;
#endif
#if defined(HAS_BATTERY)
  parameters["measured_battery"] = batt_v;
#endif

  serializeJson(result_doc, buffer, sizeof(buffer));

  StaticString<256> result_topic("%s/%s/test_result", FAC_TEST_BASE_TOPIC,
                                 app_.hw_.get_serial_number());
  mqtt_client.publish(result_topic.c_str(), buffer);

  if (passed) {
    info("factory test passed");
    prefs.begin("fac_test", false);
    prefs.clear();
    prefs.end();
    return true;
  } else {
    error("factory test failed");
    return false;
  }
}

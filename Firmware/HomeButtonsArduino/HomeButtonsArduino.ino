#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFiManager.h>
#include <Wire.h>
#include "Adafruit_SHTC3.h"

#include "config.h"
#include "display.h"

// ------ PIN definitions ------
#define BTN1_PIN 1
#define BTN2_PIN 2
#define BTN3_PIN 3
#define BTN4_PIN 4
#define BTN5_PIN 5
#define BTN6_PIN 6

#define LED1_PIN 15 
#define LED2_PIN 16
#define LED3_PIN 17
#define LED4_PIN 37
#define LED5_PIN 38
#define LED6_PIN 45

#define SDA 10
#define SCL 11
#define VBAT_ADC 14
#define CHARGER_STDBY 12
#define BOOST_EN 13

#define EINK_CS 5
#define EINK_DC 8
#define EINK_RST 9
#define EINK_BUSY 7

// ------ LED analog parameters ------
#define LED1_CH 0
#define LED2_CH 1
#define LED3_CH 2
#define LED4_CH 3
#define LED5_CH 4
#define LED6_CH 5


#define LED_RES 8
#define LED_FREQ 1000
#define LED_BRIGHT_DFLT 20

// ------ battery reading ------
#define BAT_RES_BITS 12
const float BATT_DIVIDER = 0.5;
const float BATT_ADC_REF_VOLT = 2.6;
const float MIN_BATT_VOLT = 3.3;
const float BATT_HISTERESIS_VOLT = 3.5;
const float WARN_BATT_VOLT = 3.5;
const float BATT_FULL_VOLT = 4.2;
const float BATT_EMPTY_VOLT = 3.3;

// ------ wakeup ------
#define BTN_BITMASK 0x7E // = hex of 2^PIN1 + 2^PIN2 + ...
const uint32_t TIMER_SLEEP_USEC = 600000000L;

// ------ constants ------
const char DEVICE_MODEL[] = "HomeButtons v0.1";
const char SW_VERSION[] = "v0.2.2";
const char MANUFACTURER[] = "Planinsek Industries";
const char SERIAL_NUMBER[] = "0000";
const char RANDOM_ID[] = "000000";

// ------ defaults ------
const char DEVICE_NAME[] = "Home Buttons 001";
const uint16_t MQTT_PORT = 1883;
const char BASE_TOPIC[] = "homebuttons";
const char DISCOVERY_PREFIX[] = "homeassistant";
const char BTN_1_TXT[] = "B1";
const char BTN_2_TXT[] = "B2";
const char BTN_3_TXT[] = "B3";
const char BTN_4_TXT[] = "B4";
const char BTN_5_TXT[] = "B5";
const char BTN_6_TXT[] = "B6";

const char BTN_PRESS_PAYLOAD[] = "PRESS";
const uint32_t QUICK_WIFI_TIMEOUT = 5000L;
const uint32_t WIFI_TIMEOUT = 10000L;
const uint32_t MQTT_TIMEOUT = 10000L;
const uint32_t SHORT_PRESS_TIME = 100;
const uint32_t MEDIUM_PRESS_TIME = 2000L;
const uint32_t LONG_PRESS_TIME = 10000L;
const uint32_t EXTRA_LONG_PRESS_TIME = 20000L;
const uint32_t ULTRA_LONG_PRESS_TIME = 30000L;
const uint32_t CONFIG_TIMEOUT = 300; //s
const uint32_t MQTT_DISCONNECT_TIMEOUT = 1000;
const uint32_t INFO_SCREEN_DISP_TIME = 20000L;

const char WIFI_MANAGER_TITLE[] = "Home Buttons";
const char AP_PASSWORD[] = "password123";

// ------ factory preferences ------
String device_model;
String sw_version;
String manufacturer;
String serial_number;
String unique_id;

// ------ persisted variables ------
bool low_batt_mode;
bool wifi_done;
bool setup_done;
String client_id;
bool wifi_quick_connect;
String button_1_press_topic;
String button_2_press_topic;
String button_3_press_topic;
String button_4_press_topic;
String button_5_press_topic;
String button_6_press_topic;
String temperature_topic;
String humidity_topic;
String battery_topic;

// ------ user configurable preferences ------
String device_name;
String mqtt_server;
uint32_t mqtt_port;
String mqtt_user;
String mqtt_password;
String base_topic;
String discovery_prefix;
String button_1_text;
String button_2_text;
String button_3_text;
String button_4_text;
String button_5_text;
String button_6_text;

// ------ global variables ------
uint32_t wifi_start_time = 0;
uint32_t mqtt_start_time = 0;
uint32_t info_screen_start_time = 0;

WiFiManager wifi_manager;
WiFiClient wifi_client;
PubSubClient client(wifi_client);
Preferences preferences;

bool web_portal_saved = false;

// temp & humidity sensor
TwoWire shtc3_wire = TwoWire(0);
Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();

WiFiManagerParameter device_name_param("device_name", "Device name", "", 20);
WiFiManagerParameter mqtt_server_param("mqtt_server", "MQTT Server", "", 50);
WiFiManagerParameter mqtt_port_param("mqtt_port", "MQTT Port", "", 6);
WiFiManagerParameter mqtt_user_param("mqtt_user", "MQTT User", "", 50);
WiFiManagerParameter mqtt_password_param("mqtt_password", "MQTT Password", "", 50);
WiFiManagerParameter base_topic_param("base_topic", "Base Topic", "", 50);
WiFiManagerParameter discovery_prefix_param("disc_prefix", "Discovery Prefix", "", 50);
WiFiManagerParameter btn1_txt_param("btn1_txt", "BTN1 Text", "", 20);
WiFiManagerParameter btn2_txt_param("btn2_txt", "BTN2 Text", "", 20);
WiFiManagerParameter btn3_txt_param("btn3_txt", "BTN3 Text", "", 20);
WiFiManagerParameter btn4_txt_param("btn4_txt", "BTN4 Text", "", 20);
WiFiManagerParameter btn5_txt_param("btn5_txt", "BTN5 Text", "", 20);
WiFiManagerParameter btn6_txt_param("btn6_txt", "BTN6 Text", "", 20);

enum BootReason {NO_REASON, TMR, RST, BTN, BTN_MEDIUM, BTN_LONG, BTN_EXTRA_LONG, BTN_ULTRA_LONG};
BootReason boot_reason = NO_REASON;
enum ControlFlow {NO_FLOW, WIFI_SETUP, SETUP, RESET, BTN_PRESS, FAC_RESET, TIMER};
ControlFlow control_flow = NO_FLOW;
uint32_t btn_press_time = 0;
uint32_t config_start_time = 0;
enum WakeupButton {NO_BTN, BTN1, BTN2, BTN3, BTN4, BTN5, BTN6};
WakeupButton wakeup_button = NO_BTN;
int16_t wakeup_pin = -1;

void setup() {
  // ------ hardware config ------
  pinMode(BTN1_PIN, INPUT);
  pinMode(BTN2_PIN, INPUT);
  pinMode(BTN3_PIN, INPUT);
  pinMode(BTN4_PIN, INPUT);
  pinMode(BTN5_PIN, INPUT);
  pinMode(BTN6_PIN, INPUT);

  // ------ init display ------
  display.init(); // must be before ledAttachPin (reserves GPIO37 = SPIDQS)

  ledcSetup(LED1_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED1_PIN, LED1_CH);

  ledcSetup(LED2_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED2_PIN, LED2_CH);

  ledcSetup(LED3_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED3_PIN, LED3_CH);

  ledcSetup(LED4_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED4_PIN, LED4_CH);

  ledcSetup(LED5_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED5_PIN, LED5_CH);

  ledcSetup(LED6_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED6_PIN, LED6_CH);

  // battery voltage adc
  analogReadResolution(BAT_RES_BITS);
  analogSetPinAttenuation(VBAT_ADC, ADC_11db);

  // ------ delay ------
  delay(100); // wait for peripherals to boot up

  // ------ read preferences ------
  read_preferences();
  save_factory_preferences(); // added for future compatibility

  // ------ check battery voltage first thing ------
  float batt_volt = read_battery_voltage();
  uint8_t batt_pct = batt_volt2percent(batt_volt);
  if (low_batt_mode) {
    if (batt_volt > BATT_HISTERESIS_VOLT) {
      low_batt_mode = false;
      display_buttons();
    }
    else {
      go_to_sleep();
    }
  }
  else {
    if (batt_volt < MIN_BATT_VOLT) {
      eink::display_turned_off_please_recharge_screen();
      low_batt_mode = true;
      save_preferences();
      go_to_sleep();
    }
  }

  // ------ read temp & humidity ------
  shtc3_wire.begin(SDA, SCL);
  shtc3.begin(&shtc3_wire);
  sensors_event_t humidity_event, temp_event;
  shtc3.getEvent(&humidity_event, &temp_event);
  shtc3.sleep(true);
  float temperature_meas = temp_event.temperature;
  float humidity_meas = humidity_event.relative_humidity;

  // ------ boot reason ------
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1) {
    btn_press_time = millis();
    int16_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
    wakeup_pin = (log(GPIO_reason))/log(2);
    int16_t ctrl = 0;
    bool break_loop = false;
    while (!break_loop) {
      switch (ctrl) {
        case 0: // time less than LONG_PRESS_TIME
          if (!digitalRead(wakeup_pin)) {
            boot_reason = BTN;
            ctrl = -1;
          }
          else if (millis() - btn_press_time > MEDIUM_PRESS_TIME) {
            eink::display_info_screen(temperature_meas, humidity_meas, batt_volt2percent(batt_volt));
            info_screen_start_time = millis();
            ctrl = 1;
          }
          break;
        case 1:
          if (!digitalRead(wakeup_pin)) {
            delay(100); // debounce
            ctrl = 2;
          }
          else if (millis() - btn_press_time > LONG_PRESS_TIME) {
            eink::display_string("Release\nfor\nSETUP\n\nKeep holding\nfor\nWiFi SETUP");
            ctrl = 3;
          }
          break;
        case 2: // show info screen until timeout or btn press
          if (digitalReadAny() || millis() - info_screen_start_time > INFO_SCREEN_DISP_TIME) {
            boot_reason = BTN_MEDIUM;
            ctrl = -1;
            display_buttons();
          }
          break;
        case 3:
          if (!digitalRead(wakeup_pin)) {
            boot_reason = BTN_LONG;
            ctrl = -1;
          }
          else if (millis() - btn_press_time > EXTRA_LONG_PRESS_TIME) {
            ctrl = 4;
            eink::display_string("Release\nfor\nWiFi SETUP\n\nKeep holding\nfor\nFACTORY\nRESET");
          }
          break;
        case 4:
          if (!digitalRead(wakeup_pin)) {
            boot_reason = BTN_EXTRA_LONG;
            ctrl = -1;
          }
          else if (millis() - btn_press_time > ULTRA_LONG_PRESS_TIME) {
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
  }
  else if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
    boot_reason = TMR;
  }
  else {
    boot_reason = RST;
  }


  switch (boot_reason) {
    case NO_REASON: control_flow = NO_FLOW; break;
    case RST:
      if (!wifi_done) control_flow = WIFI_SETUP;
      else if (!setup_done) control_flow = SETUP;
      else control_flow = RESET;
      break;
    case TMR:
      if (!wifi_done && !setup_done) control_flow = NO_FLOW;
      else control_flow = TIMER;
      break;
    case BTN:
      if (!wifi_done) control_flow = WIFI_SETUP;
      else if (!setup_done) control_flow = SETUP;
      else control_flow = BTN_PRESS;
      break;
    case BTN_MEDIUM: control_flow = NO_FLOW; break;
    case BTN_LONG:
      if (!wifi_done) control_flow = WIFI_SETUP;
      else control_flow = SETUP;
      break;
    case BTN_EXTRA_LONG:  control_flow = WIFI_SETUP; break;
    case BTN_ULTRA_LONG: control_flow = FAC_RESET; break;
  }

  // ------ main logic switch ------
  switch (control_flow) {
    case WIFI_SETUP: {
      delay(50); // debounce
      eink::display_string("Starting\nWiFi setup...");
      wifi_quick_connect = false;
      config_start_time = millis();
      // start wifi manager
      String ap_name = String("HB-") + RANDOM_ID;
      eink::display_ap_config_screen(ap_name, AP_PASSWORD);

      WiFi.mode(WIFI_STA);
      wifi_manager.setConfigPortalTimeout(CONFIG_TIMEOUT);
      wifi_manager.resetSettings();
      wifi_manager.setTitle(WIFI_MANAGER_TITLE);
      wifi_manager.setBreakAfterConfig(true);
      wifi_manager.setDarkMode(true);
    
      bool wifi_connected = wifi_manager.startConfigPortal(ap_name.c_str(), AP_PASSWORD);
      bool wifi_connected_2 = connect_wifi();

      if (!wifi_connected && !wifi_connected_2) { // double check that wifi can not be connected with connect_wifi() function
        wifi_done = false;
        eink::display_please_complete_setup_screen(unique_id.c_str());
        break;
      }

      wifi_done = true;
      save_preferences(); // save now because restarting before end of setup funciton
      eink::display_wifi_connected_screen();
      delay(3000);
      ESP.restart();
      break;
    }
    case SETUP: {
      delay(50); // debounce
      eink::display_string("Starting\nsetup...");
      bool wifi_connected = connect_wifi();
      if (!wifi_connected) {
        eink::display_error("WiFi\nerror");
        delay(3000);
        if (setup_done) {
          display_buttons();
        }
        else {
          eink::display_please_complete_setup_screen(unique_id.c_str());
        }
        break;
      }
      wifi_quick_connect = false; // maybe user changed wifi settings
      eink::display_web_config_screen(WiFi.localIP().toString());
      config_start_time = millis();

      wifi_manager.setTitle(WIFI_MANAGER_TITLE);
      wifi_manager.setSaveParamsCallback(save_params_clbk);
      wifi_manager.setBreakAfterConfig(true);
      wifi_manager.setShowPassword(true);
      wifi_manager.setParamsPage(true);
      wifi_manager.setDarkMode(true);

      // Parameters
      device_name_param.setValue(device_name.c_str(), 20);
      mqtt_server_param.setValue(mqtt_server.c_str(), 50);
      mqtt_port_param.setValue(String(mqtt_port).c_str(), 6);
      mqtt_user_param.setValue(mqtt_user.c_str(), 50);
      mqtt_password_param.setValue(mqtt_password.c_str(), 50);
      base_topic_param.setValue(base_topic.c_str(), 50);
      discovery_prefix_param.setValue(discovery_prefix.c_str(), 50);
      btn1_txt_param.setValue(button_1_text.c_str(), 20);
      btn2_txt_param.setValue(button_2_text.c_str(), 20);
      btn3_txt_param.setValue(button_3_text.c_str(), 20);
      btn4_txt_param.setValue(button_4_text.c_str(), 20);
      btn5_txt_param.setValue(button_5_text.c_str(), 20);
      btn6_txt_param.setValue(button_6_text.c_str(), 20);
      wifi_manager.addParameter(&device_name_param);
      wifi_manager.addParameter(&mqtt_server_param);
      wifi_manager.addParameter(&mqtt_port_param);
      wifi_manager.addParameter(&mqtt_user_param);
      wifi_manager.addParameter(&mqtt_password_param);
      wifi_manager.addParameter(&base_topic_param);
      wifi_manager.addParameter(&discovery_prefix_param);
      wifi_manager.addParameter(&btn1_txt_param);
      wifi_manager.addParameter(&btn2_txt_param);
      wifi_manager.addParameter(&btn3_txt_param);
      wifi_manager.addParameter(&btn4_txt_param);
      wifi_manager.addParameter(&btn5_txt_param);
      wifi_manager.addParameter(&btn6_txt_param);

      web_portal_saved = false; // set to true in web portal save button callback
      wifi_manager.startWebPortal();
      while (millis() - config_start_time < CONFIG_TIMEOUT*1000L) {
        wifi_manager.process();
        if (digitalReadAny() || web_portal_saved) {
          break;
        }
      }
      wifi_manager.stopWebPortal();

      bool mqtt_connected = connect_mqtt();
      if (!mqtt_connected) {
        setup_done = false;
        eink::display_error("MQTT\nerror");
        delay(3000);
        eink::display_please_complete_setup_screen(unique_id.c_str());
        break;
      }
      else {
        set_topics();
        send_autodiscovery_msg();
      }
      setup_done = true;
      eink::display_setup_complete_screen();
      delay(3000);
      display_buttons();
      break;
    }
    case BTN_PRESS: {
      switch (wakeup_pin) {
        case BTN1_PIN: wakeup_button = BTN1;
          set_led(LED1_CH, LED_BRIGHT_DFLT);
        break;
        case BTN2_PIN: wakeup_button = BTN3;
          set_led(LED2_CH, LED_BRIGHT_DFLT);
        break;
        case BTN3_PIN: wakeup_button = BTN5;
          set_led(LED3_CH, LED_BRIGHT_DFLT);
        break;
        case BTN4_PIN: wakeup_button = BTN6;
          set_led(LED4_CH, LED_BRIGHT_DFLT);
        break;
        case BTN5_PIN: wakeup_button = BTN4;
          set_led(LED5_CH, LED_BRIGHT_DFLT);
        break;
        case BTN6_PIN: wakeup_button = BTN2;
          set_led(LED6_CH, LED_BRIGHT_DFLT);
        break;
      }
      bool wifi_connected = connect_wifi();
      if (!wifi_connected) {
        eink::display_error("WiFi\nerror");
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
          client.publish(button_1_press_topic.c_str(), BTN_PRESS_PAYLOAD);
          break;
        case BTN2:
          client.publish(button_2_press_topic.c_str(), BTN_PRESS_PAYLOAD);
          break;
        case BTN3:
          client.publish(button_3_press_topic.c_str(), BTN_PRESS_PAYLOAD);
          break;
        case BTN4:
          client.publish(button_4_press_topic.c_str(), BTN_PRESS_PAYLOAD);
          break;
        case BTN5:
          client.publish(button_5_press_topic.c_str(), BTN_PRESS_PAYLOAD);
          break;
        case BTN6:
          client.publish(button_6_press_topic.c_str(), BTN_PRESS_PAYLOAD);
          break;
      }
      client.publish(temperature_topic.c_str(), String(temperature_meas).c_str());
      client.publish(humidity_topic.c_str(), String(humidity_meas).c_str());
      client.publish(battery_topic.c_str(), String(batt_pct).c_str());
      if (batt_volt < WARN_BATT_VOLT) {
        eink::display_please_recharge_soon_screen();
        delay(2000);
        display_buttons();
      }
      break;
    }
    case RESET: {
      eink::display_string("RESET");
      if (wifi_done && setup_done) 
        display_buttons();
      else
        eink::display_please_complete_setup_screen(unique_id.c_str());
      break;
    }
    case FAC_RESET: {
      eink::display_string("Factory\nRESET");
      clear_preferences();
      wifi_manager.resetSettings();
      delay(3000);
      ESP.restart();
      break;
    }
    case TIMER: {
      bool wifi_connected = connect_wifi();
      if (!wifi_connected) {
        break;
      }
      bool mqtt_connected = connect_mqtt();
      if (!mqtt_connected) {
        break;
      }
      client.publish(temperature_topic.c_str(), String(temperature_meas).c_str());
      client.publish(humidity_topic.c_str(), String(humidity_meas).c_str());
      client.publish(battery_topic.c_str(), String(batt_pct).c_str());
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
  save_preferences();
  disconnect_mqtt();
  go_to_sleep();
}

void loop() {
  // will never get here :)
}

bool connect_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);

  // Try to connect with settings stored by ESP32
  WiFi.begin();
  wifi_start_time = millis();
  while (true) {
      delay(50);
      if (WiFi.status() == WL_CONNECTED) {
        if (wifi_quick_connect) {
          return true;
        }
        else {
          break;
        }
      }
      else if (millis() - wifi_start_time > QUICK_WIFI_TIMEOUT) {
        break;  // proceed with normal connection retry
      }
  }

  // If not successful connect only with SSID and password, and then save new channel & BSSID info
  String ssid = WiFi.SSID();  // returns empty string if WiFi.begin() is not called prior!
  String psk = WiFi.psk();
  WiFi.begin(ssid.c_str(), psk.c_str());
  wifi_start_time = millis();
  delay(2000);
  while (true) {
      delay(50);
      if (WiFi.status() == WL_CONNECTED) {
        uint8_t * bssid = WiFi.BSSID();
        int32_t ch = WiFi.channel();
        WiFi.disconnect();
        WiFi.begin(ssid.c_str(), psk.c_str(), ch, bssid, true);
        wifi_start_time = millis();
        while (true) {
          delay(50);
          if (WiFi.status() == WL_CONNECTED) {
            wifi_quick_connect = true; // can connect next time with params saved by esp
            return true;
          }
          else if (millis() - wifi_start_time > WIFI_TIMEOUT) {
            wifi_quick_connect = false; // make sure next time quick connect is disabled
            return false;
          }
        }
      }
      else if (millis() - wifi_start_time > WIFI_TIMEOUT) {
        wifi_quick_connect = false; // make sure next time quick connect is disabled
        return false;
      }
  }
}

bool connect_mqtt() {
  client.setServer(mqtt_server.c_str(), mqtt_port);
  client.setBufferSize(2048);
  mqtt_start_time = millis();
  while(!client.connected()) {
    if (mqtt_user.length() > 0 && mqtt_password.length() > 0) {
      client.connect(client_id.c_str(), mqtt_user.c_str(), mqtt_password.c_str());
    }
    else {
      client.connect(client_id.c_str());
    }
    delay(50);
    if(millis() - mqtt_start_time > MQTT_TIMEOUT) {
      return false;
    }
  }
  return true;
}

void read_preferences() {
  preferences.begin("factory", true);
  device_model = preferences.getString("device_model", DEVICE_MODEL);
  sw_version = preferences.getString("sw_version", SW_VERSION);
  manufacturer = preferences.getString("manufacturer", MANUFACTURER);
  serial_number = preferences.getString("serial_number", SERIAL_NUMBER);
  unique_id = preferences.getString("unique_id", String("HBTNS-") + serial_number + "-" + RANDOM_ID);
  preferences.end();

  preferences.begin("user", true);
  low_batt_mode = preferences.getBool("lb_mode", false);
  wifi_done = preferences.getBool("wifi_done", false);
  setup_done = preferences.getBool("setup_done", false);
  device_name = preferences.getString("device_name", DEVICE_NAME);
  mqtt_server = preferences.getString("mqtt_srv", "");
  mqtt_port = preferences.getUInt("mqtt_port", MQTT_PORT);
  mqtt_user = preferences.getString("mqtt_user", "");
  mqtt_password = preferences.getString("mqtt_pass", "");
  wifi_quick_connect = preferences.getBool("wifi_qc", false);
  client_id = preferences.getString("client_id", "");
  base_topic = preferences.getString("base_topic", BASE_TOPIC);
  discovery_prefix = preferences.getString("disc_prefix", DISCOVERY_PREFIX);
  button_1_press_topic = preferences.getString("btn1_p_topic", "");
  button_2_press_topic = preferences.getString("btn2_p_topic", "");
  button_3_press_topic = preferences.getString("btn3_p_topic", "");
  button_4_press_topic = preferences.getString("btn4_p_topic", "");
  button_5_press_topic = preferences.getString("btn5_p_topic", "");
  button_6_press_topic = preferences.getString("btn6_p_topic", "");
  temperature_topic = preferences.getString("tmp_topic", "");
  humidity_topic = preferences.getString("humd_topic", "");
  battery_topic = preferences.getString("batt_topic", "");
  button_1_text = preferences.getString("btn1_txt", BTN_1_TXT);
  button_2_text = preferences.getString("btn2_txt", BTN_2_TXT);
  button_3_text = preferences.getString("btn3_txt", BTN_3_TXT);
  button_4_text = preferences.getString("btn4_txt", BTN_4_TXT);
  button_5_text = preferences.getString("btn5_txt", BTN_5_TXT);
  button_6_text = preferences.getString("btn6_txt", BTN_6_TXT);
  preferences.end();
}

void save_factory_preferences() {
  preferences.begin("factory", false);
  preferences.putString("device_model", device_model);
  preferences.putString("sw_version", sw_version);
  preferences.putString("manufacturer", manufacturer);
  preferences.putString("serial_number", serial_number);
  preferences.putString("unique_id", unique_id);
  preferences.end();
}

void save_preferences() {
  preferences.begin("user", false);
  preferences.putBool("lb_mode", low_batt_mode);
  preferences.putBool("wifi_done", wifi_done);
  preferences.putBool("setup_done", setup_done);
  preferences.putString("device_name", device_name);
  preferences.putString("mqtt_srv", mqtt_server);
  preferences.putUInt("mqtt_port", mqtt_port);
  preferences.putString("mqtt_user", mqtt_user);
  preferences.putString("mqtt_pass", mqtt_password);
  preferences.putString("client_id", client_id);
  preferences.putBool("wifi_qc", wifi_quick_connect);
  preferences.putString("base_topic", base_topic);
  preferences.putString("disc_prefix", discovery_prefix);
  preferences.putString("btn1_p_topic", button_1_press_topic);
  preferences.putString("btn2_p_topic", button_2_press_topic);
  preferences.putString("btn3_p_topic", button_3_press_topic);
  preferences.putString("btn4_p_topic", button_4_press_topic);
  preferences.putString("btn5_p_topic", button_5_press_topic);
  preferences.putString("btn6_p_topic", button_6_press_topic);
  preferences.putString("tmp_topic", temperature_topic);
  preferences.putString("humd_topic", humidity_topic);
  preferences.putString("batt_topic", battery_topic);
  preferences.putString("btn1_txt", button_1_text);
  preferences.putString("btn2_txt", button_2_text);
  preferences.putString("btn3_txt", button_3_text);
  preferences.putString("btn4_txt", button_4_text);
  preferences.putString("btn5_txt", button_5_text);
  preferences.putString("btn6_txt", button_6_text);
  preferences.end();
}

void clear_factory_preferences() {
  preferences.begin("factory", false);
  preferences.clear();
  preferences.end();
}

void clear_preferences() {
  preferences.begin("user", false);
  preferences.clear();
  preferences.end();
}

void disconnect_mqtt() {
  // Wait a bit
  unsigned long tm = millis();
  while (millis() - tm < MQTT_DISCONNECT_TIMEOUT) {
    client.loop();
  }
  // disconnect and wait until closed
  client.disconnect(); 
  wifi_client.flush();
  // wait until connection is closed completely
  while( client.state() != -1){  
    client.loop();
    delay(10);
  }
}

void start_esp_sleep() {
  esp_sleep_enable_ext1_wakeup(BTN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
  if (wifi_done && setup_done && !low_batt_mode) {
      esp_sleep_enable_timer_wakeup(TIMER_SLEEP_USEC);
  }
  esp_deep_sleep_start();
}

void go_to_sleep() {
  eink::hibernate();
  set_all_leds(0);
  start_esp_sleep();
}

void set_topics() {
  String button_topic_common = base_topic + "/" + device_name + "/";
  button_1_press_topic = button_topic_common + "button_1";
  button_2_press_topic = button_topic_common + "button_2";
  button_3_press_topic = button_topic_common + "button_3";
  button_4_press_topic = button_topic_common + "button_4";
  button_5_press_topic = button_topic_common + "button_5";
  button_6_press_topic = button_topic_common + "button_6";
  temperature_topic = button_topic_common + "temperature";
  humidity_topic = button_topic_common + "humidity";
  battery_topic = button_topic_common + "battery";
}

void send_autodiscovery_msg() {
  // Construct topics
  String trigger_topic_common = discovery_prefix + "/device_automation/" + unique_id;
  String button_1_config_topic = trigger_topic_common + "/button1/config";
  String button_2_config_topic = trigger_topic_common + "/button2/config";
  String button_3_config_topic = trigger_topic_common + "/button3/config";
  String button_4_config_topic = trigger_topic_common + "/button4/config";
  String button_5_config_topic = trigger_topic_common + "/button5/config";
  String button_6_config_topic = trigger_topic_common + "/button6/config";
  String sensor_topic_common = discovery_prefix + "/sensor/" + unique_id;
  String temperature_config_topic = sensor_topic_common + "/temperature/config";
  String humidity_config_topic = sensor_topic_common + "/humidity/config";
  String battery_config_topic = sensor_topic_common + "/battery/config";

  // Construct autodiscovery config json
  DynamicJsonDocument btn_1_conf(2048);
  btn_1_conf["automation_type"] = "trigger";
  btn_1_conf["topic"] = button_1_press_topic;
  btn_1_conf["payload"] = BTN_PRESS_PAYLOAD;
  btn_1_conf["type"] = "button_short_press";
  btn_1_conf["subtype"] = "button_1";
  JsonObject device1 = btn_1_conf.createNestedObject("device");
  device1["identifiers"][0] = unique_id;
  device1["model"] = device_model;
  device1["name"] = device_name;
  device1["sw_version"] = sw_version;
  device1["manufacturer"] = manufacturer;

  DynamicJsonDocument btn_2_conf(2048);
  btn_2_conf["automation_type"] = "trigger";
  btn_2_conf["topic"] = button_2_press_topic;
  btn_2_conf["payload"] = BTN_PRESS_PAYLOAD;
  btn_2_conf["type"] = "button_short_press";
  btn_2_conf["subtype"] = "button_2";
  JsonObject device2 = btn_2_conf.createNestedObject("device");
  device2["identifiers"][0] = unique_id;

  DynamicJsonDocument btn_3_conf(2048);
  btn_3_conf["automation_type"] = "trigger";
  btn_3_conf["topic"] = button_3_press_topic;
  btn_3_conf["payload"] = BTN_PRESS_PAYLOAD;
  btn_3_conf["type"] = "button_short_press";
  btn_3_conf["subtype"] = "button_3";
  JsonObject device3 = btn_3_conf.createNestedObject("device");
  device3["identifiers"][0] = unique_id;

  DynamicJsonDocument btn_4_conf(2048);
  btn_4_conf["automation_type"] = "trigger";
  btn_4_conf["topic"] = button_4_press_topic;
  btn_4_conf["payload"] = BTN_PRESS_PAYLOAD;
  btn_4_conf["type"] = "button_short_press";
  btn_4_conf["subtype"] = "button_4";
  JsonObject device4 = btn_4_conf.createNestedObject("device");
  device4["identifiers"][0] = unique_id;

  DynamicJsonDocument btn_5_conf(2048);
  btn_5_conf["automation_type"] = "trigger";
  btn_5_conf["topic"] = button_5_press_topic;
  btn_5_conf["payload"] = BTN_PRESS_PAYLOAD;
  btn_5_conf["type"] = "button_short_press";
  btn_5_conf["subtype"] = "button_5";
  JsonObject device5 = btn_5_conf.createNestedObject("device");
  device5["identifiers"][0] = unique_id;

  DynamicJsonDocument btn_6_conf(2048);
  btn_6_conf["automation_type"] = "trigger";
  btn_6_conf["topic"] = button_6_press_topic;
  btn_6_conf["payload"] = BTN_PRESS_PAYLOAD;
  btn_6_conf["type"] = "button_short_press";
  btn_6_conf["subtype"] = "button_6";
  JsonObject device6 = btn_6_conf.createNestedObject("device");
  device6["identifiers"][0] = unique_id;

  DynamicJsonDocument temp_conf(2048);
  temp_conf["name"] = device_name + " Temperature";
  temp_conf["unique_id"] = unique_id + "_temperature";
  temp_conf["state_topic"] = temperature_topic;
  temp_conf["device_class"] = "temperature";
  temp_conf["unit_of_measurement"] = "°C";
  temp_conf["expire_after"] = "660";
  JsonObject device7 = temp_conf.createNestedObject("device");
  device7["identifiers"][0] = unique_id;

  DynamicJsonDocument humidity_conf(2048);
  humidity_conf["name"] = device_name + " Humidity";
  humidity_conf["unique_id"] = unique_id + "_humidity";
  humidity_conf["state_topic"] = humidity_topic;
  humidity_conf["device_class"] = "humidity";
  humidity_conf["unit_of_measurement"] = "%";
  humidity_conf["expire_after"] = "660";
  JsonObject device8 = humidity_conf.createNestedObject("device");
  device8["identifiers"][0] = unique_id;

  DynamicJsonDocument battery_conf(2048);
  battery_conf["name"] = device_name + " Battery";
  battery_conf["unique_id"] = unique_id + "_battery";
  battery_conf["state_topic"] = battery_topic;
  battery_conf["device_class"] = "battery";
  battery_conf["unit_of_measurement"] = "%";
  battery_conf["expire_after"] = "660";
  JsonObject device9 = battery_conf.createNestedObject("device");
  device9["identifiers"][0] = unique_id;

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

void display_buttons() {
  eink::display_buttons(
    button_1_text.c_str(),
    button_2_text.c_str(),
    button_3_text.c_str(),
    button_4_text.c_str(),
    button_5_text.c_str(),
    button_6_text.c_str()
  );
}

void save_params_clbk() {
  device_name = device_name_param.getValue();
  mqtt_server = mqtt_server_param.getValue();
  mqtt_port = String(mqtt_port_param.getValue()).toInt();
  mqtt_user = mqtt_user_param.getValue();
  mqtt_password = mqtt_password_param.getValue();
  base_topic = base_topic_param.getValue();
  discovery_prefix = discovery_prefix_param.getValue();
  button_1_text = btn1_txt_param.getValue();
  button_2_text = btn2_txt_param.getValue();
  button_3_text = btn3_txt_param.getValue();
  button_4_text = btn4_txt_param.getValue();
  button_5_text = btn5_txt_param.getValue();
  button_6_text = btn6_txt_param.getValue();
  web_portal_saved = true;
}

bool digitalReadAny() {
  return digitalRead(BTN1_PIN) || digitalRead(BTN2_PIN) ||
         digitalRead(BTN3_PIN) || digitalRead(BTN4_PIN) ||
         digitalRead(BTN5_PIN) || digitalRead(BTN6_PIN);
}

void set_led(uint8_t ch, uint8_t brightness) {
  ledcWrite(ch, brightness);
}

void set_all_leds(uint8_t brightness) {
  set_led(LED1_CH, brightness);
  set_led(LED2_CH, brightness);
  set_led(LED3_CH, brightness);
  set_led(LED4_CH, brightness);
  set_led(LED5_CH, brightness);
  set_led(LED6_CH, brightness);
}

float read_battery_voltage() {
  return analogRead(VBAT_ADC) / 4095.0 * BATT_ADC_REF_VOLT / BATT_DIVIDER;
}

uint8_t batt_volt2percent(float volt) {
    float pct = 100 * (volt - BATT_EMPTY_VOLT)/(BATT_FULL_VOLT - BATT_EMPTY_VOLT);
    if (pct < 0.0) pct = 0;
    else if (pct > 100.0) pct = 100;
    return (uint8_t) round(pct);
}
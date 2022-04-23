#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFiManager.h>

#include "config.h"
#include "display.h"

// ------ PIN definitions ------
#define LED_PIN 15
#define BTN1_PIN 1
#define BTN2_PIN 2
#define BTN3_PIN 4
#define BTN4_PIN 6

#define SDA 10
#define SCL 11
#define VBAT_ADC 14
#define CHARGER_STDBY 12
#define BOOST_EN 13

// ------ wakeup source bitmask ------
#define BTN_BITMASK 0x56 // gpio 1, 2, 4, 6

// ------ constants ------
const char DEVICE_MODEL[] = "HomeButtons v0.1";
const char SW_VERSION[] = "v0.1";
const char MANUFACTURER[] = "Planinsek Industries";
const char SERIAL_NUMBER[] = "000001";
const char UNIQUE_ID[] = "5eab";

// ------ defaults ------
const char DEVICE_NAME[] = "Home Buttons 001";
const uint16_t MQTT_PORT = 1883;
const char BASE_TOPIC[] = "homebuttons";
const char DISCOVERY_PREFIX[] = "homeassistant";
const char BTN_1_TXT[] = "B1";
const char BTN_2_TXT[] = "B2";
const char BTN_3_TXT[] = "B3";
const char BTN_4_TXT[] = "B4";

const char BTN_PRESS_PAYLOAD[] = "PRESS";
const uint32_t WIFI_TIMEOUT = 10000L;
const uint32_t MQTT_TIMEOUT = 10000L;
const uint32_t SHORT_PRESS_TIME = 100;
const uint32_t LONG_PRESS_TIME = 10000L;
const uint32_t EXTRA_LONG_PRESS_TIME = 20000L;
const uint32_t ULTRA_LONG_PRESS_TIME = 30000L;
const uint32_t CONFIG_TIMEOUT = 300; //s
const uint32_t MQTT_DISCONNECT_TIMEOUT = 1000;

const char WIFI_MANAGER_TITLE[] = "Home Buttons";
// const char AP_NAME[] = "HomeButtons";
const char AP_PASSWORD[] = "password123";

// ------ constants for testing ------
const char MQTT_SERVER[] = "192.168.1.8";

// ------ factory preferences ------
String device_model;
String sw_version;
String manufacturer;
String serial_number;
String unique_id;

// ------ persisted variables ------
bool initialized;
bool wifi_done;
bool setup_done;
String client_id;
String button_1_press_topic;
String button_2_press_topic;
String button_3_press_topic;
String button_4_press_topic;

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

// ------ global variables ------
uint32_t wifi_start_time = 0;
uint32_t mqtt_start_time = 0;

WiFiManager wifi_manager;
WiFiClient wifi_client;
PubSubClient client(wifi_client);
Preferences preferences;

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

enum BootReason {NO_REASON, INITIAL, RST, BTN, BTN_LONG, BTN_EXTRA_LONG, BTN_ULTRA_LONG};
BootReason boot_reason = NO_REASON;
enum ControlFlow {NO_FLOW, WIFI_SETUP, SETUP, RESET, BTN_PRESS, FAC_RESET};
ControlFlow control_flow = NO_FLOW;
uint32_t btn_press_time = 0;
uint32_t config_start_time = 0;
enum WakeupButton {NO_BTN, BTN1, BTN2, BTN3, BTN4};
WakeupButton wakeup_button = NO_BTN;
int16_t wakeup_pin = -1;

void setup() {
  // ------ hardware config ------
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN1_PIN, INPUT);
  pinMode(BTN2_PIN, INPUT);
  pinMode(BTN3_PIN, INPUT);
  pinMode(BTN4_PIN, INPUT);
  Serial.begin(115200);
  display.init();

  // eink::display_ap_config_screen(AP_NAME, AP_PASSWORD);
  // eink::display_web_config_screen("192.168.1.29");
  // return;


  // ------ read preferences ------
  // clear_preferences();
  read_preferences();

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
          else if (millis() - btn_press_time > LONG_PRESS_TIME) {
            ctrl = 1;
            eink::display_string("Release\nfor\nWEB CONFIG\n\nHold\nfor\nAP");
          }
          break;
        case 1:
          if (!digitalRead(wakeup_pin)) {
            boot_reason = BTN_LONG;
            ctrl = -1;
          }
          else if (millis() - btn_press_time > EXTRA_LONG_PRESS_TIME) {
            ctrl = 2;
            eink::display_string("Release\nfor\nAP\n\nHold\nfor\nRESET");
          }
          break;
        case 2:
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
  // else if (initialized) {
  //   boot_reason = RST;
  // }
  else {
    boot_reason = RST;
  }




  if (!wifi_done) {
    control_flow = WIFI_SETUP;
  }
  else if (!setup_done) {
    control_flow = SETUP;
  }
  else {
    switch (boot_reason) {
      // case INITIAL: control_flow = WIFI_SETUP; break;
      case RST: control_flow = RESET; break;
      case BTN: control_flow = BTN_PRESS; break;
      case BTN_LONG: control_flow = SETUP; break;
      case BTN_EXTRA_LONG:  control_flow = WIFI_SETUP; break;
      case BTN_ULTRA_LONG: control_flow = FAC_RESET; break;
    }
  }

  // ------ main logic switch ------
  switch (control_flow) {
    case WIFI_SETUP: {
      delay(50); // debounce
      config_start_time = millis();
      // start wifi manager
      String ap_name = String("HB-") + unique_id;
      eink::display_ap_config_screen(ap_name, AP_PASSWORD);

      WiFi.mode(WIFI_STA);
      wifi_manager.setConfigPortalTimeout(CONFIG_TIMEOUT);

      wifi_manager.setTitle(WIFI_MANAGER_TITLE);
      wifi_manager.setBreakAfterConfig(true);
      wifi_manager.setDarkMode(true);
    
      bool wifi_connected = wifi_manager.startConfigPortal(ap_name.c_str(), AP_PASSWORD);

      if (!wifi_connected) {
        eink::display_wifi_not_connected_screen();
        break;
      }

      wifi_done = true;

      save_preferences();
      eink::display_wifi_connected_screen();
      delay(3000);
      ESP.restart();
      break;
    }
    case SETUP: {
      delay(50); // debounce
      bool wifi_connected = connect_wifi();
      if (!wifi_connected) {
        eink::display_error("WIFI\nnot connected");
        break;
      }
      eink::display_web_config_screen(WiFi.localIP().toString());
      config_start_time = millis();

      wifi_manager.setTitle(WIFI_MANAGER_TITLE);
      wifi_manager.setSaveParamsCallback(save_params);
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

      wifi_manager.startWebPortal();
      while (millis() - config_start_time < CONFIG_TIMEOUT*1000L) {
        wifi_manager.process();
        if (digitalReadAny()) {
          break;
        }
      }
      wifi_manager.stopWebPortal();
      setup_done = true;
      save_preferences();

      bool mqtt_connected = connect_mqtt();
      if (!mqtt_connected) {
        eink::display_error("MQTT\nfailed");
        break;
      }
      else {
        send_autodiscovery_msg();
      }

      eink::display_setup_complete_screen();
      delay(3000);
      display_buttons();
      break;
    }
    case BTN_PRESS: {
      switch (wakeup_pin) {
        case BTN1_PIN: wakeup_button = BTN1; break;
        case BTN2_PIN: wakeup_button = BTN2; break;
        case BTN3_PIN: wakeup_button = BTN3; break;
        case BTN4_PIN: wakeup_button = BTN4; break;
      }
      bool wifi_connected = connect_wifi();
      if (!wifi_connected) {
        eink::display_error("WIFI\nfailed");
        delay(3000);
        display_buttons();
        break;
      }
      bool mqtt_connected = connect_mqtt();
      if (!mqtt_connected) {
        eink::display_error("MQTT\nfailed");
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
      }
      eink::display_ok_screen();
      display_buttons();
      break;
    }
    case RESET: {
      eink::display_string("RESET");
      display_buttons();
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
    default: {
      break;
    }
  }

  eink::hibernate();
  go_to_sleep();
}

void loop() {
  // will never get here :)
}

bool connect_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(); // ssid & password saved by esp32 internally
  wifi_start_time = millis();
  while (WiFi.status() != WL_CONNECTED) {
      delay(50);
      if (millis() - wifi_start_time > WIFI_TIMEOUT) {
        return false;
      }
  }
  return true;
}

bool connect_mqtt() {
  client.setServer(mqtt_server.c_str(), mqtt_port);
  client.setBufferSize(2048);
  mqtt_start_time = millis();
  while(!client.connected()) {
    client.connect(client_id.c_str());
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
  unique_id = preferences.getString("unique_id", UNIQUE_ID);
  preferences.end();

  preferences.begin("user", true);
  initialized = preferences.getBool("initialized", false);
  wifi_done = preferences.getBool("wifi_done", false);
  setup_done = preferences.getBool("setup_done", false);
  device_name = preferences.getString("device_name", DEVICE_NAME);
  mqtt_server = preferences.getString("mqtt_srv", "");
  mqtt_port = preferences.getUInt("mqtt_port", MQTT_PORT);
  mqtt_user = preferences.getString("mqtt_user", "");
  mqtt_password = preferences.getString("mqtt_pass", "");
  client_id = preferences.getString("client_id", "");
  base_topic = preferences.getString("base_topic", BASE_TOPIC);
  discovery_prefix = preferences.getString("disc_prefix", DISCOVERY_PREFIX);
  button_1_press_topic = preferences.getString("btn1_p_topic", "");
  button_2_press_topic = preferences.getString("btn2_p_topic", "");
  button_3_press_topic = preferences.getString("btn3_p_topic", "");
  button_4_press_topic = preferences.getString("btn4_p_topic", "");
  button_1_text = preferences.getString("btn1_txt", BTN_1_TXT);
  button_2_text = preferences.getString("btn2_txt", BTN_2_TXT);
  button_3_text = preferences.getString("btn3_txt", BTN_3_TXT);
  button_4_text = preferences.getString("btn4_txt", BTN_4_TXT);
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
  preferences.putBool("initialized", initialized);
  preferences.putBool("wifi_done", wifi_done);
  preferences.putBool("setup_done", setup_done);
  preferences.putString("device_name", device_name);
  preferences.putString("mqtt_srv", mqtt_server);
  preferences.putUInt("mqtt_port", mqtt_port);
  preferences.putString("mqtt_user", mqtt_user);
  preferences.putString("mqtt_pass", mqtt_password);
  preferences.putString("client_id", client_id);
  preferences.putString("base_topic", base_topic);
  preferences.putString("disc_prefix", discovery_prefix);
  preferences.putString("btn1_p_topic", button_1_press_topic);
  preferences.putString("btn2_p_topic", button_2_press_topic);
  preferences.putString("btn3_p_topic", button_3_press_topic);
  preferences.putString("btn4_p_topic", button_4_press_topic);
  preferences.putString("btn1_txt", button_1_text);
  preferences.putString("btn2_txt", button_2_text);
  preferences.putString("btn3_txt", button_3_text);
  preferences.putString("btn4_txt", button_4_text);
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

void go_to_sleep() {
  disconnect_mqtt();
  esp_sleep_enable_ext1_wakeup(BTN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
}

void set_topics() {
  client_id = String("HBTNS-") + device_name + "-" + unique_id;
  String button_topic_common = base_topic + "/" + device_name + "/";
  button_1_press_topic = button_topic_common + "button_1";
  button_2_press_topic = button_topic_common + "button_2";
  button_3_press_topic = button_topic_common + "button_3";
  button_4_press_topic = button_topic_common + "button_4";
}

void send_autodiscovery_msg() {
  String u_id = String("HBTNS-") + serial_number + "-" + unique_id;

  // Construct topics
  String config_topic_common = discovery_prefix + "/device_automation/" + u_id;
  String button_1_config_topic = config_topic_common + "/button1/config";
  String button_2_config_topic = config_topic_common + "/button2/config";
  String button_3_config_topic = config_topic_common + "/button3/config";
  String button_4_config_topic = config_topic_common + "/button4/config";

  // Construct autodiscovery config json
  DynamicJsonDocument btn_1_conf(2048);
  btn_1_conf["automation_type"] = "trigger";
  btn_1_conf["topic"] = button_1_press_topic;
  btn_1_conf["payload"] = BTN_PRESS_PAYLOAD;
  btn_1_conf["type"] = "button_short_press";
  btn_1_conf["subtype"] = "button_1";
  JsonObject device1 = btn_1_conf.createNestedObject("device");
  device1["identifiers"][0] = serial_number;
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
  device2["identifiers"][0] = serial_number;

  DynamicJsonDocument btn_3_conf(2048);
  btn_3_conf["automation_type"] = "trigger";
  btn_3_conf["topic"] = button_3_press_topic;
  btn_3_conf["payload"] = BTN_PRESS_PAYLOAD;
  btn_3_conf["type"] = "button_short_press";
  btn_3_conf["subtype"] = "button_3";
  JsonObject device3 = btn_3_conf.createNestedObject("device");
  device3["identifiers"][0] = serial_number;

  DynamicJsonDocument btn_4_conf(2048);
  btn_4_conf["automation_type"] = "trigger";
  btn_4_conf["topic"] = button_4_press_topic;
  btn_4_conf["payload"] = BTN_PRESS_PAYLOAD;
  btn_4_conf["type"] = "button_short_press";
  btn_4_conf["subtype"] = "button_4";
  JsonObject device4 = btn_4_conf.createNestedObject("device");
  device4["identifiers"][0] = serial_number;

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
}

void display_buttons() {
  eink::display_buttons(
    button_1_text.c_str(),
    button_2_text.c_str(),
    button_3_text.c_str(),
    button_4_text.c_str()
  );
}

void save_params() {
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
  set_topics();
  save_preferences();

}

bool digitalReadAny() {
  return digitalRead(BTN1_PIN) || digitalRead(BTN2_PIN) ||
         digitalRead(BTN3_PIN) || digitalRead(BTN4_PIN);
}
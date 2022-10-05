#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ------ device ------
const char MANUFACTURER[] = "Planinsek Industries";
const char SW_VERSION[] = "v1.0.0";
const char DOCS_LINK[] = "https://docs.home-buttons.com";

// ------ wifi AP ------
const char WIFI_MANAGER_TITLE[] = "Home Buttons";
const char AP_PASSWORD[] = "password123";

// ------ constants ------
const char BTN_PRESS_PAYLOAD[] = "PRESS";

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

// ----- timing ------
const uint32_t QUICK_WIFI_TIMEOUT = 5000L;
const uint32_t WIFI_TIMEOUT = 10000L;
const uint32_t MQTT_TIMEOUT = 10000L;
const uint32_t SHORT_PRESS_TIME = 100;
const uint32_t MEDIUM_PRESS_TIME = 2000L;
const uint32_t LONG_PRESS_TIME = 10000L;
const uint32_t EXTRA_LONG_PRESS_TIME = 20000L;
const uint32_t ULTRA_LONG_PRESS_TIME = 30000L;
const uint32_t CONFIG_TIMEOUT = 600;  // s
const uint32_t MQTT_DISCONNECT_TIMEOUT = 1000;
const uint32_t INFO_SCREEN_DISP_TIME = 30000L;  // ms
const uint32_t SENSOR_PUBLISH_TIME = 600000;    // ms

#endif
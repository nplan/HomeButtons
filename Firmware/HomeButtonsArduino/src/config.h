#ifndef A15CA4BD_C231_407C_B7AF_C4ED245559A6
#define A15CA4BD_C231_407C_B7AF_C4ED245559A6

#include <Arduino.h>

// ------ device ------
const char MANUFACTURER[] = "PLab";
const char SW_VERSION[] = "v2.0.3-beta";
const char DOCS_LINK[] = "https://docs.home-buttons.com/setup";

// ------ wifi AP ------
const char WIFI_MANAGER_TITLE[] = "Home Buttons";
const char SETUP_AP_PASSWORD[] = "password123";

// ------ buttons ------
const uint8_t NUM_BUTTONS = 6;
const char BTN_PRESS_PAYLOAD[] = "PRESS";
const uint8_t BTN_LABEL_MAXLEN = 15;

// ------ defaults ------
const char DEVICE_NAME_DFLT[] = "Home Buttons";
const uint16_t MQTT_PORT_DFLT = 1883;
const char BASE_TOPIC_DFLT[] = "homebuttons";
const char DISCOVERY_PREFIX_DFLT[] = "homeassistant";
const char BTN_1_LABEL_DFLT[] = "B1";
const char BTN_2_LABEL_DFLT[] = "B2";
const char BTN_3_LABEL_DFLT[] = "B3";
const char BTN_4_LABEL_DFLT[] = "B4";
const char BTN_5_LABEL_DFLT[] = "B5";
const char BTN_6_LABEL_DFLT[] = "B6";

// ------ sensors ------
const uint16_t SEN_INTERVAL_DFLT = 10; // min
const uint16_t SEN_INTERVAL_MIN = 1; // min
const uint16_t SEN_INTERVAL_MAX = 30; // min

// ----- timing ------
const uint32_t SETUP_TIMEOUT = 600;  // s
const uint32_t INFO_SCREEN_DISP_TIME = 30000L;  // ms
const uint32_t AWAKE_SENSOR_INTERVAL = 60000L;  // ms
const uint32_t WDT_TIMEOUT_AWAKE = 60;  // s
const uint32_t WDT_TIMEOUT_SLEEP = 60;  // s
const uint32_t AWAKE_REDRAW_INTERVAL = 1000L;  //ms

// ------ network ------
const uint32_t QUICK_WIFI_TIMEOUT = 5000L;
const uint32_t WIFI_TIMEOUT = 20000L;
const uint32_t MQTT_TIMEOUT = 15000L;
const uint32_t MQTT_DISCONNECT_TIMEOUT = 1000L;
const uint32_t NET_CONN_CHECK_INTERVAL = 1000L;
const uint32_t NET_CONNECT_TIMEOUT = 45000L;
const uint8_t MAX_FAILED_CONNECTIONS = 5;
const uint16_t MQTT_PYLD_SIZE = 512;
const uint16_t MQTT_BUFFER_SIZE = 777;

// ------ LEDs ------
const uint8_t LED_DFLT_BRIGHT = 225;

// ------ other ------
const uint32_t MIN_FREE_HEAP = 20000UL;

#endif /* A15CA4BD_C231_407C_B7AF_C4ED245559A6 */

#ifndef HOMEBUTTONS_CONFIG_H
#define HOMEBUTTONS_CONFIG_H

#include <WString.h>

// ------ device ------
static constexpr char MANUFACTURER[] = "PLab";
static constexpr char SW_VERSION[] = "v2.0.7-beta";
static constexpr char DOCS_LINK[] = "https://docs.home-buttons.com/setup";

// ------ wifi AP ------
static constexpr char WIFI_MANAGER_TITLE[] = "Home Buttons";
static constexpr char SETUP_AP_PASSWORD[] = "password123";

// ------ buttons ------
static constexpr uint8_t NUM_BUTTONS = 6;
static constexpr char BTN_PRESS_PAYLOAD[] = "PRESS";
static constexpr uint8_t BTN_LABEL_MAXLEN = 15;

// ------ defaults ------
static constexpr char DEVICE_NAME_DFLT[] = "Home Buttons";
static constexpr uint16_t MQTT_PORT_DFLT = 1883;
static constexpr char BASE_TOPIC_DFLT[] = "homebuttons";
static constexpr char DISCOVERY_PREFIX_DFLT[] = "homeassistant";
static constexpr char BTN_1_LABEL_DFLT[] = "B1";
static constexpr char BTN_2_LABEL_DFLT[] = "B2";
static constexpr char BTN_3_LABEL_DFLT[] = "B3";
static constexpr char BTN_4_LABEL_DFLT[] = "B4";
static constexpr char BTN_5_LABEL_DFLT[] = "B5";
static constexpr char BTN_6_LABEL_DFLT[] = "B6";

// ------ sensors ------
static constexpr uint16_t SEN_INTERVAL_DFLT = 10;  // min
static constexpr uint16_t SEN_INTERVAL_MIN = 1;    // min
static constexpr uint16_t SEN_INTERVAL_MAX = 30;   // min

// ----- timing ------
static constexpr uint32_t SETUP_TIMEOUT = 600;             // s
static constexpr uint32_t INFO_SCREEN_DISP_TIME = 30000L;  // ms
static constexpr uint32_t AWAKE_SENSOR_INTERVAL = 60000L;  // ms
static constexpr uint32_t WDT_TIMEOUT_AWAKE = 60;          // s
static constexpr uint32_t WDT_TIMEOUT_SLEEP = 60;          // s
static constexpr uint32_t AWAKE_REDRAW_INTERVAL = 1000L;   // ms

// ------ network ------
static constexpr uint32_t QUICK_WIFI_TIMEOUT = 5000L;
static constexpr uint32_t WIFI_TIMEOUT = 20000L;
static constexpr uint32_t MAX_WIFI_RETRIES_DURING_MQTT_SETUP = 5;
static constexpr uint32_t MQTT_TIMEOUT = 15000L;
static constexpr uint32_t MQTT_DISCONNECT_TIMEOUT = 1000L;
static constexpr uint32_t NET_CONN_CHECK_INTERVAL = 1000L;
static constexpr uint32_t NET_CONNECT_TIMEOUT = 45000L;
static constexpr uint8_t MAX_FAILED_CONNECTIONS = 5;

// ------ LEDs ------
static constexpr uint8_t LED_DFLT_BRIGHT = 225;

// ------ other ------
static constexpr uint32_t MIN_FREE_HEAP = 20000UL;

#endif  // HOMEBUTTONS_CONFIG_H

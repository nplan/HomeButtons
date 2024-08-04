#ifndef HOMEBUTTONS_CONFIG_H
#define HOMEBUTTONS_CONFIG_H

#if !defined(HOME_BUTTONS_ORIGINAL) && !defined(HOME_BUTTONS_MINI) && \
    !defined(HOME_BUTTONS_PRO)
#error "No device defined!"
#endif

#include <WString.h>
#include <IPAddress.h>

// ------ device ------
static constexpr char MANUFACTURER[] = "PLab";
static constexpr char SW_VERSION[] = "v2.5.0-alpha2";

#if defined(HOME_BUTTONS_ORIGINAL)
static constexpr char SW_MODEL_ID[] = "A1";
#elif defined(HOME_BUTTONS_MINI)
static constexpr char SW_MODEL_ID[] = "B1";
#elif defined(HOME_BUTTONS_PRO)
static constexpr char SW_MODEL_ID[] = "C1";
#endif

// ------ URLs ------
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_PRO)
static constexpr char DOCS_LINK[] =
    "https://docs.home-buttons.com/original/setup/";
#elif defined(HOME_BUTTONS_MINI)
static constexpr char DOCS_LINK[] = "https://docs.home-buttons.com/mini/setup/";
#elif defined(HOME_BUTTONS_PRO)
static constexpr char DOCS_LINK[] = "https://docs.home-buttons.com/pro/setup/";
#endif

// ------ wifi AP ------
static constexpr char WIFI_MANAGER_TITLE[] = "Home Buttons";
static constexpr char SETUP_AP_PASSWORD[] = "password123";

// ------ buttons ------
#if defined(HOME_BUTTONS_ORIGINAL)
static constexpr uint8_t NUM_BUTTONS = 6;
#elif defined(HOME_BUTTONS_MINI)
static constexpr uint8_t NUM_BUTTONS = 4;
#elif defined(HOME_BUTTONS_PRO)
static constexpr uint8_t NUM_BUTTONS = 9;
#endif
static constexpr char BTN_PRESS_PAYLOAD[] = "PRESS";
static constexpr uint8_t BTN_LABEL_MAXLEN = 56;
static constexpr uint8_t USER_MSG_MAXLEN = 64;

// ------ defaults ------
static constexpr char DEVICE_NAME_DFLT[] = "Home Buttons";
static constexpr uint16_t MQTT_PORT_DFLT = 1883;
static constexpr char BASE_TOPIC_DFLT[] = "homebuttons";
static constexpr char DISCOVERY_PREFIX_DFLT[] = "homeassistant";
static constexpr char BNT_LABEL_DFLT_PREFIX[] = "B";

// ------ sensors ------
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_PRO)
static constexpr uint16_t SEN_INTERVAL_DFLT = 10;  // min
static constexpr uint16_t SEN_INTERVAL_MIN = 1;    // min
static constexpr uint16_t SEN_INTERVAL_MAX = 30;   // min
#elif defined(HOME_BUTTONS_MINI)
static constexpr uint16_t SEN_INTERVAL_DFLT = 30;  // min
static constexpr uint16_t SEN_INTERVAL_MIN = 5;    // min
static constexpr uint16_t SEN_INTERVAL_MAX = 60;   // min
#endif

// ----- timing ------
static constexpr uint32_t SETUP_TIMEOUT = 600;              // s
static constexpr uint32_t INFO_SCREEN_DISP_TIME = 30000L;   // ms
static constexpr uint32_t AWAKE_SENSOR_INTERVAL = 15000L;   // ms
static constexpr uint32_t WDT_TIMEOUT_AWAKE = 60;           // s
static constexpr uint32_t WDT_TIMEOUT_SLEEP = 60;           // s
static constexpr uint32_t AWAKE_REDRAW_INTERVAL = 1000L;    // ms
static constexpr uint32_t SETTINGS_MENU_TIMEOUT = 300000L;  // ms
static constexpr uint32_t DEVICE_INFO_TIMEOUT = 30000L;     // ms
static constexpr uint32_t SHUTDOWN_DELAY = 500L;            // ms
static constexpr uint32_t FRONTLIGHT_TIMEOUT = 5000L;       // ms

// ------ network ------
static constexpr uint32_t QUICK_WIFI_TIMEOUT = 5000L;
static constexpr uint32_t WIFI_TIMEOUT = 20000L;
static constexpr uint32_t MAX_WIFI_RETRIES_DURING_MQTT_SETUP = 2;
static constexpr uint32_t MQTT_TIMEOUT = 5000L;
static constexpr uint32_t NET_CONN_CHECK_INTERVAL = 1000L;
static constexpr uint32_t NET_CONNECT_TIMEOUT = 30000L;
static constexpr uint8_t MAX_FAILED_CONNECTIONS = 5;
static const IPAddress DEFAULT_DNS2 = IPAddress(1, 1, 1, 1);

// ------ other ------
static constexpr uint32_t MIN_FREE_HEAP = 10000UL;
static constexpr uint32_t SCHEDULE_WAKEUP_MIN = 5;                      // s
static constexpr uint32_t SCHEDULE_WAKEUP_MAX = SEN_INTERVAL_MAX * 60;  // s
static constexpr uint32_t MDI_FREE_SPACE_THRESHOLD = 100000UL;

// ------ UI ------
#if defined(HOME_BUTTONS_ORIGINAL)
static constexpr char BATT_EMPTY_MSG[] =
    "Battery\nLOW\n\nPlease\nrecharge\nsoon!";
#elif defined(HOME_BUTTONS_MINI)
static constexpr char BATT_EMPTY_MSG[] =
    "Batteries\nLOW\n\nPlease\nreplace\nsoon!";
#endif

#endif  // HOMEBUTTONS_CONFIG_H

#ifndef HOMEBUTTONS_CONFIG_H
#define HOMEBUTTONS_CONFIG_H

// #define LED_DEFAULT_FADE_TIME 100

#if !defined(HOME_BUTTONS_ORIGINAL) && !defined(HOME_BUTTONS_MINI) && \
    !defined(HOME_BUTTONS_PRO) && !defined(HOME_BUTTONS_INDUSTRIAL)
#error "No device defined!"
#endif

#if defined(HOME_BUTTONS_ORIGINAL)
#define HAS_BUTTON_UI
#define HAS_DISPLAY
#define HAS_BATTERY
#define HAS_CHARGER
#define HAS_AWAKE_MODE
#define HAS_SLEEP_MODE
#define HAS_TH_SENSOR
#endif

#if defined(HOME_BUTTONS_MINI)
#define HAS_BUTTON_UI
#define HAS_DISPLAY
#define HAS_BATTERY
#define HAS_SLEEP_MODE
#define HAS_TH_SENSOR
#endif

#if defined(HOME_BUTTONS_PRO)
#define HAS_TOUCH_UI
#define HAS_DISPLAY
#define HAS_FRONTLIGHT
#define HAS_TH_SENSOR
#define HAS_FRONTLIGHT
#endif

#if defined(HOME_BUTTONS_INDUSTRIAL)
#define HAS_BUTTON_UI
#endif

#include <WString.h>
#include <IPAddress.h>

// ------ device ------
static constexpr char MANUFACTURER[] = "PLab";
static constexpr char SW_VERSION[] = "v2.6.0";

#if defined(HOME_BUTTONS_ORIGINAL)
static constexpr char SW_MODEL_ID[] = "A1";
#elif defined(HOME_BUTTONS_MINI)
static constexpr char SW_MODEL_ID[] = "B1";
#elif defined(HOME_BUTTONS_PRO)
static constexpr char SW_MODEL_ID[] = "C1";
#elif defined(HOME_BUTTONS_INDUSTRIAL)
static constexpr char SW_MODEL_ID[] = "D1";
#endif

// ------ URLs ------
#if defined(HOME_BUTTONS_ORIGINAL)
static constexpr char DOCS_LINK[] =
    "https://docs.home-buttons.com/original/setup/";
#elif defined(HOME_BUTTONS_MINI)
static constexpr char DOCS_LINK[] = "https://docs.home-buttons.com/mini/setup/";
#elif defined(HOME_BUTTONS_PRO)
static constexpr char DOCS_LINK[] = "https://docs.home-buttons.com/pro/setup/";
#elif defined(HOME_BUTTONS_INDUSTRIAL)
static constexpr char DOCS_LINK[] =
    "https://docs.home-buttons.com/industrial/setup/";
#endif
static constexpr char ICON_URL_DFLT[] = "https://icons.home-buttons.com/mdi/";

// ------ wifi AP ------
static constexpr char SETUP_AP_PASSWORD[] = "password123";

// ------ buttons ------
#if defined(HOME_BUTTONS_ORIGINAL)
static constexpr uint8_t NUM_BUTTONS = 6;
#elif defined(HOME_BUTTONS_MINI)
static constexpr uint8_t NUM_BUTTONS = 4;
#elif defined(HOME_BUTTONS_PRO)
static constexpr uint8_t NUM_BUTTONS = 9;
#elif defined(HOME_BUTTONS_INDUSTRIAL)
static constexpr uint8_t NUM_BUTTONS = 5;
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
static constexpr char BTN_CONF_DFLT[] = "BBBBBBBBBBBBBBBB";

// ------ sensors ------
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_PRO) || \
    defined(HOME_BUTTONS_INDUSTRIAL)
static constexpr uint16_t SEN_INTERVAL_DFLT = 10;  // min
static constexpr uint16_t SEN_INTERVAL_MIN = 1;    // min
static constexpr uint16_t SEN_INTERVAL_MAX = 30;   // min
#elif defined(HOME_BUTTONS_MINI)
static constexpr uint16_t SEN_INTERVAL_DFLT = 30;  // min
static constexpr uint16_t SEN_INTERVAL_MIN = 5;    // min
static constexpr uint16_t SEN_INTERVAL_MAX = 60;   // min
#endif

// ----- timing ------
static constexpr uint32_t SETUP_TIMEOUT = 600;                // s
static constexpr uint32_t INFO_SCREEN_DISP_TIME = 15000L;     // ms
static constexpr uint32_t AWAKE_SENSOR_INTERVAL = 15000L;     // ms
static constexpr uint32_t WDT_TIMEOUT_AWAKE = 60;             // s
static constexpr uint32_t WDT_TIMEOUT_SLEEP = 60;             // s
static constexpr uint32_t AWAKE_REDRAW_INTERVAL = 1000L;      // ms
static constexpr uint32_t SETTINGS_MENU_TIMEOUT = 30000L;     // ms
static constexpr uint32_t DEVICE_INFO_TIMEOUT = 30000L;       // ms
static constexpr uint32_t SHUTDOWN_DELAY = 500L;              // ms
static constexpr uint32_t FRONTLIGHT_TIMEOUT = 5000L;         // ms
static constexpr uint32_t SLEEP_MODE_INPUT_TIMEOUT = 10000L;  // ms

// ------ network ------
static constexpr uint32_t QUICK_WIFI_TIMEOUT = 5000L;
static constexpr uint32_t WIFI_TIMEOUT = 20000L;
static constexpr uint32_t MAX_WIFI_RETRIES_DURING_MQTT_SETUP = 2;
static constexpr uint32_t MQTT_TIMEOUT = 5000L;
static constexpr uint32_t NET_CONN_CHECK_INTERVAL = 1000L;
static constexpr uint32_t NET_CONNECT_TIMEOUT = 30000L;
static constexpr uint8_t MAX_FAILED_CONNECTIONS = 5;
static const IPAddress DEFAULT_DNS2 = IPAddress(1, 1, 1, 1);

// ------ MQTT ------
static constexpr uint16_t MQTT_PYLD_SIZE = 512;
static constexpr uint16_t MQTT_BUFFER_SIZE = 777;
static constexpr size_t MAX_TOPIC_LENGTH = 256;

// ------ other ------
static constexpr uint32_t MIN_FREE_HEAP = 10000UL;
static constexpr uint32_t SCHEDULE_WAKEUP_MIN = 5;                      // s
static constexpr uint32_t SCHEDULE_WAKEUP_MAX = SEN_INTERVAL_MAX * 60;  // s
static constexpr uint32_t MDI_FREE_SPACE_THRESHOLD = 100000UL;
static constexpr uint16_t LED_DEFAULT_FADE_TIME = 50;  // ms

// ------ UI ------
#if defined(HOME_BUTTONS_ORIGINAL)
static constexpr char BATT_EMPTY_MSG[] =
    "Battery\nLOW\n\nPlease\nrecharge\nsoon!";
#elif defined(HOME_BUTTONS_MINI)
static constexpr char BATT_EMPTY_MSG[] =
    "Batteries\nLOW\n\nPlease\nreplace\nsoon!";
#endif

// ------ LEDs ------
static constexpr uint8_t LED_DFLT_BRIGHT = 100;    // pct
static constexpr uint8_t LED_MIN_BRIGHT = 10;      // pct
static constexpr uint8_t LED_MAX_AMB_BRIGHT = 20;  // pct
static constexpr float LED_GAMMA = 2.2;

// ------ BUTTONS ------
static constexpr uint32_t kBtnDebounceTimeout = 50L;
static constexpr uint32_t kBtnPressTimeout = 500L;
static constexpr uint32_t kBtnTriggerInterval = 250L;

#endif  // HOMEBUTTONS_CONFIG_H

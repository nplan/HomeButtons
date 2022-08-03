#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ------ constants ------
const char MANUFACTURER[] = "Planinsek Industries";
const char SW_VERSION[] = "v0.4.0";

// ------ wifi AP ------
const char WIFI_MANAGER_TITLE[] = "Home Buttons";
const char AP_PASSWORD[] = "password123";

// ------ wakeup ------
const uint32_t TIMER_SLEEP_USEC = 600000000L;

#endif
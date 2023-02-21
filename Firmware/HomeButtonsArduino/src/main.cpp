#include <Arduino.h>
#include <WiFiManager.h>
#include <esp_task_wdt.h>

#include "app.h"
#include "autodiscovery.h"
#include "buttons.h"
#include "config.h"
#include "display.h"
#include "factory.h"
#include "hardware.h"
#include "hw_tests.h"
#include "leds.h"
#include "network.h"
#include "setup.h"
#include "state.h"

static App app;

void setup() { app.setup(); }  // end setup()

void loop() {}

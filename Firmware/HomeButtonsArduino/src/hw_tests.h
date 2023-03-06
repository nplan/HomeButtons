#ifndef HOMEBUTTONS_HW_TESTS_H
#define HOMEBUTTONS_HW_TESTS_H

#include <WiFi.h>

#include "hardware.h"

namespace hw_tests {
void blink_leds(String hw_version);
void led_on_button(String hw_version);
void wifi_stress(String hw_version);
}  // namespace hw_tests

#endif  // HOMEBUTTONS_HW_TESTS_H
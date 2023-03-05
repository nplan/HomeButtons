#ifndef HOMEBUTTONS_HW_TESTS_H
#define HOMEBUTTONS_HW_TESTS_H

#include <WiFi.h>

#include "hardware.h"
#include "state.h"

namespace hw_tests {
void blink_leds(const DeviceState::HWVersion& hw_version);
void led_on_button(const DeviceState::HWVersion& hw_version);
void wifi_stress(const DeviceState::HWVersion& hw_version);
}  // namespace hw_tests

#endif  // HOMEBUTTONS_HW_TESTS_H
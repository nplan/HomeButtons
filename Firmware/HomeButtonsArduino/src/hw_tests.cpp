#include "hw_tests.h"

void hw_tests::blink_leds(const DeviceState::HWVersion& hw_version) {
  HW.init(hw_version);
  HW.begin();
  while (1) {
    for (int m = 0; m < 10; m++) {
      for (int n = 0; n <= 255; n++) {
        ledcWrite(HW.LED1_CH, n);
        ledcWrite(HW.LED2_CH, n);
        ledcWrite(HW.LED3_CH, n);
        ledcWrite(HW.LED4_CH, n);
        ledcWrite(HW.LED5_CH, n);
        ledcWrite(HW.LED6_CH, n);
        delay(1);
      }
      for (int n = 255; n >= 0; n--) {
        ledcWrite(HW.LED1_CH, n);
        ledcWrite(HW.LED2_CH, n);
        ledcWrite(HW.LED3_CH, n);
        ledcWrite(HW.LED4_CH, n);
        ledcWrite(HW.LED5_CH, n);
        ledcWrite(HW.LED6_CH, n);
        delay(1);
      }
    }
  }
}

void hw_tests::led_on_button(const DeviceState::HWVersion& hw_version) {
  HW.init(hw_version);
  HW.begin();
  while (1) {
    HW.set_led(HW.LED3_CH, digitalRead(HW.BTN3_PIN) ? 255 : 0);
    HW.set_led(HW.LED2_CH, digitalRead(HW.BTN2_PIN) ? 255 : 0);
    HW.set_led(HW.LED4_CH, digitalRead(HW.BTN4_PIN) ? 255 : 0);
    HW.set_led(HW.LED1_CH, digitalRead(HW.BTN1_PIN) ? 255 : 0);
    HW.set_led(HW.LED5_CH, digitalRead(HW.BTN5_PIN) ? 255 : 0);
    HW.set_led(HW.LED6_CH, digitalRead(HW.BTN6_PIN) ? 255 : 0);
  }
}

void hw_tests::wifi_stress(const DeviceState::HWVersion& hw_version) {
  HW.init(hw_version);
  HW.begin();
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.begin("notexistingssid", "notexistingpassword");
  while (1) {
    HW.set_all_leds(255);
    delay(500);
    HW.set_all_leds(0);
    delay(500);
  }
}
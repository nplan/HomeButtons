#include <WiFi.h>

#include "hardware.h"

namespace hw_tests {

void blink_leds(String hw_version) {
  init_hardware(hw_version);
  begin_hardware();
  while (1) {
    for (int n = 0; n < 10; n++) {
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

void led_on_button(String hw_version) {
  init_hardware(hw_version);
  begin_hardware();
  while (1) {
    set_led(HW.LED1_CH, digitalRead(HW.BTN1_PIN) ? 255 : 0);
    set_led(HW.LED2_CH, digitalRead(HW.BTN2_PIN) ? 255 : 0);
    set_led(HW.LED3_CH, digitalRead(HW.BTN3_PIN) ? 255 : 0);
    set_led(HW.LED4_CH, digitalRead(HW.BTN4_PIN) ? 255 : 0);
    set_led(HW.LED5_CH, digitalRead(HW.BTN5_PIN) ? 255 : 0);
    set_led(HW.LED6_CH, digitalRead(HW.BTN6_PIN) ? 255 : 0);
  }
}

void wifi_stress(String hw_version) {
  init_hardware(hw_version);
  begin_hardware();
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.begin("notexistingssid", "notexistingpassword");
  while (1) {
    set_all_leds(255);
    delay(500);
    set_all_leds(0);
    delay(500);
  }
}

}  // namespace hw_tests
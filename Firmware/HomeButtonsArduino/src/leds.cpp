#include "leds.h"

#include "hardware.h"

void LEDs::begin() {
  state = State::ACTIVE;
  log_i("[LEDS] begin");
}

void LEDs::end() {
  cmd_state = CMDState::CMD_END;
  log_d("[LEDS] cmd end");
}

void LEDs::blink(uint8_t led_num, uint8_t num_blinks, bool hold,
                 uint8_t brightness) {
  if (num_blinks < 1)
    num_blinks = 1;
  else if (num_blinks > 5)
    num_blinks = 5;
  Blink blink{.led_num = led_num,
              .num_blinks = num_blinks,
              .brightness = brightness,
              .hold = hold};
  cmd_blink = blink;
  new_cmd_blink = true;
  log_d("[LEDS] blink - led: %d, blinks: %d, bri: %d, hold: %d", led_num,
        num_blinks, brightness, hold);
}

LEDs::State LEDs::get_state() const { return state; }

void LEDs::update() {
  if (state != State::ACTIVE) {
    return;
  }

  if (new_cmd_blink) {
    current_blink = cmd_blink;
    cmd_blink = {};
    new_cmd_blink = false;
    uint16_t on, off;
    switch (current_blink.num_blinks) {
      case 1:
        on = 400;
        off = 400;
        break;
      case 2:
        on = 100;
        off = 300;
        break;
      case 3:
        on = 67;
        off = 200;
        break;
      case 4:
        on = 50;
        off = 150;
        break;
      default:
        on = 50;
        off = 150;
        break;
    }
    for (uint8_t i = 0; i < current_blink.num_blinks; i++) {
      HW.set_led_num(current_blink.led_num, current_blink.brightness);
      delay(on);
      if (!(current_blink.hold && i == current_blink.num_blinks - 1)) {
        HW.set_led_num(current_blink.led_num, 0);
        delay(off);
      }
    }
    current_blink = {};
  }
  if (cmd_state == CMDState::CMD_END) {
    cmd_state = CMDState::NONE;
    state = State::IDLE;
    log_i("[LEDS] ended");
  }
}

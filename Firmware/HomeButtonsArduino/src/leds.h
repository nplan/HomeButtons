#ifndef HOMEBUTTONS_LEDS_H
#define HOMEBUTTONS_LEDS_H

#include "Arduino.h"
#include "config.h"
#include "logger.h"

class LEDs : public Logger {
 public:
  enum class State { IDLE, ACTIVE };

  LEDs() : Logger("LEDs") {}

  void begin();
  void end();
  void blink(uint8_t led_num, uint8_t num_blinks, bool hold = false,
             uint8_t brightness = LED_DFLT_BRIGHT);
  State get_state() const;

  void update();

 private:
  enum class CMDState { NONE, CMD_END };

  struct Blink {
    uint8_t led_num = 0;
    uint8_t num_blinks = 0;
    uint8_t brightness = 0;
    bool hold = false;
  };

  State state = State::IDLE;
  CMDState cmd_state = CMDState::NONE;
  Blink cmd_blink = {};
  bool new_cmd_blink = false;
  Blink current_blink = {};
};

#endif  // HOMEBUTTONS_LEDS_H

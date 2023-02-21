#ifndef HOMEBUTTONS_BUTTONS_H
#define HOMEBUTTONS_BUTTONS_H

#include "Arduino.h"

const uint32_t LONG_1_TIME = 2000L;
const uint32_t LONG_2_TIME = 10000L;
const uint32_t LONG_3_TIME = 20000L;
const uint32_t LONG_4_TIME = 30000L;

const uint32_t DEBOUNCE_TIMEOUT = 50L;
const uint32_t PRESS_TIMEOUT = 500L;

class Button {
 public:
  enum ButtonAction {
    IDLE,
    SINGLE,
    DOUBLE,
    TRIPLE,
    QUAD,
    LONG_1,
    LONG_2,
    LONG_3,
    LONG_4
  };
  void setup(uint8_t pin, uint16_t id, bool active_high = true);
  void begin();
  void init_press();
  void end();
  void update();
  ButtonAction get_action() const;
  bool is_press_finished() const;
  void clear();
  uint8_t get_pin() const;
  uint16_t get_id() const;
  static const char* get_action_name(ButtonAction action);
  static uint8_t get_action_multi_count(ButtonAction action);

 private:
  bool begun = false;

  uint8_t pin = 0;
  uint16_t id = 0;
  bool active_high = false;

  uint8_t state_machine_state = 0;

  uint32_t press_start_time = 0;
  uint32_t release_start_time = 0;

  ButtonAction action = IDLE;
  bool press_finished = false;

  bool rising_flag = false;
  bool falling_flag = false;

  void IRAM_ATTR isr();
  bool read_pin() const;
  void reset();
};

#endif  // HOMEBUTTONS_BUTTONS_H

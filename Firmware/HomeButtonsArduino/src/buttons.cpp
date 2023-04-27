#include "buttons.h"

#include "FunctionalInterrupt.h"

void Button::begin(uint8_t pin, uint16_t id, bool active_high) {
  if (begun) return;
  if (!(LONG_1_TIME < LONG_3_TIME && LONG_2_TIME < LONG_3_TIME &&
        LONG_3_TIME < LONG_4_TIME)) {
    debug("invalid press times");
    return;
  }
  this->pin = pin;
  this->id = id;
  this->active_high = active_high;
  attachInterrupt(pin, std::bind(&Button::_isr, this), CHANGE);
  begun = true;
  debug("id %d begun", id);
}

void Button::init_press() {
  action = SINGLE;
  state_machine_state = 2;
  press_start_time = millis();
}

void Button::end() {
  if (!begun) return;
  detachInterrupt(pin);
  _reset();
  begun = false;
  debug("id %d ended", id);
}

void Button::update() {
  if (!begun || press_finished) {
    return;
  }

  uint32_t since_press_start = millis() - press_start_time;
  uint32_t since_release_start = millis() - release_start_time;

  switch (state_machine_state) {
    case 0:  // idle
      if (rising_flag) {
        rising_flag = false;
        press_start_time = millis();
        state_machine_state = 1;
      }
      break;
    case 1:  // debounce
      if (since_press_start >= DEBOUNCE_TIMEOUT) {
        if (_read_pin()) {
          action = SINGLE;
          state_machine_state = 2;
        } else {
          state_machine_state = 0;
        }
      }
      break;
    case 2:  // pin is high
      if (falling_flag || !_read_pin()) {
        falling_flag = false;
        switch (action) {
          case SINGLE:
            release_start_time = millis();
            state_machine_state = 3;
            break;
          case LONG_1:
          case LONG_2:
          case LONG_3:
          case LONG_4:
            debug("id %d press: %s", id, get_action_name(action));
            press_finished = true;
            release_start_time = millis();
            state_machine_state = 8;
            break;
          default:
            break;
        }
      }
      if (since_press_start >= LONG_1_TIME && since_press_start < LONG_2_TIME) {
        action = LONG_1;
      } else if (since_press_start >= LONG_2_TIME &&
                 since_press_start < LONG_3_TIME) {
        action = LONG_2;
      } else if (since_press_start >= LONG_3_TIME &&
                 since_press_start < LONG_4_TIME) {
        action = LONG_3;
      } else if (since_press_start >= LONG_4_TIME) {
        action = LONG_4;
      }
      break;
    case 3:  // debounce
      if (since_release_start >= DEBOUNCE_TIMEOUT) {
        state_machine_state = 4;
      }
      break;
    case 4:  // end of single, wait for additional presses
      if (rising_flag) {
        rising_flag = false;
        press_start_time = millis();
        state_machine_state = 5;
      } else if (since_press_start >= PRESS_TIMEOUT) {
        debug("id %d press: %s", id, get_action_name(action));
        press_finished = true;
        state_machine_state = 0;
      }
      break;
    case 5:  // debounce next press
      if (since_press_start >= DEBOUNCE_TIMEOUT) {
        if (_read_pin()) {
          switch (action) {
            case SINGLE:
              action = DOUBLE;
              break;
            case DOUBLE:
              action = TRIPLE;
              break;
            case TRIPLE:
              action = QUAD;
              break;
            default:
              break;
          }
          state_machine_state = 6;
        } else {
          debug("id %d press: %s", id, get_action_name(action));
          press_finished = true;
          release_start_time = millis();
          state_machine_state = 8;
        }
      }
      break;
    case 6:  // wait for release
      if (!_read_pin()) {
        release_start_time = millis();
        state_machine_state = 7;
      }
      break;
    case 7:  // debounce and return to 4
      if (since_release_start >= DEBOUNCE_TIMEOUT) {
        state_machine_state = 4;
      }
      break;
    case 8:
      if (since_release_start >= DEBOUNCE_TIMEOUT) {
        state_machine_state = 0;
      }
  }
}

Button::ButtonAction Button::get_action() const { return action; }

bool Button::is_press_finished() const { return press_finished; }

uint8_t Button::get_pin() const { return pin; }

uint16_t Button::get_id() const { return id; }

const char* Button::get_action_name(ButtonAction action) {
  switch (action) {
    case IDLE:
      return "IDLE";
    case SINGLE:
      return "SINGLE";
    case DOUBLE:
      return "DOUBLE";
    case TRIPLE:
      return "TRIPLE";
    case QUAD:
      return "QUAD";
    case LONG_1:
      return "LONG_1";
    case LONG_2:
      return "LONG_2";
    case LONG_3:
      return "LONG_3";
    case LONG_4:
      return "LONG_4";
    default:
      return "";
  }
}

uint8_t Button::get_action_multi_count(ButtonAction action) {
  uint8_t count;
  switch (action) {
    case SINGLE:
      count = 1;
      break;
    case DOUBLE:
      count = 2;
      break;
    case TRIPLE:
      count = 3;
      break;
    case QUAD:
      count = 4;
      break;
    default:
      count = 0;
  }
  return count;
}

void Button::_isr() {
  if (_read_pin()) {
    rising_flag = true;
  } else {
    falling_flag = true;
  }
}

bool Button::_read_pin() const {
  if (active_high) {
    return digitalRead(pin);
  } else {
    return !digitalRead(pin);
  }
}

void Button::_reset() {
  rising_flag = false;
  falling_flag = false;
  press_finished = false;
  state_machine_state = 0;
  action = IDLE;
}

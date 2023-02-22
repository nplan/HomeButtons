#include "buttons.h"

#include "FunctionalInterrupt.h"

void Button::begin(uint8_t pin, uint16_t id, bool active_high) {
  if (begun) return;
  if (!(LONG_1_TIME < LONG_3_TIME && LONG_2_TIME < LONG_3_TIME &&
        LONG_3_TIME < LONG_4_TIME)) {
    return;
  }
  this->pin = pin;
  this->id = id;
  this->active_high = active_high;
  attachInterrupt(pin, std::bind(&Button::isr, this), CHANGE);
  begun = true;
  log_d("[BTN] id %d begun", id);
}

void Button::init_press() {
  action = SINGLE;
  state_machine_state = 2;
  press_start_time = millis();
}

void Button::end() {
  if (!begun) return;
  detachInterrupt(pin);
  reset();
  begun = false;
  log_d("[BTN] id %d ended", id);
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
        if (read_pin()) {
          action = SINGLE;
          state_machine_state = 2;
        } else {
          state_machine_state = 0;
        }
      }
      break;
    case 2:  // pin is high
      if (falling_flag || !read_pin()) {
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
            log_d("[BTN] id %d press: %s",id, get_action_name(action));
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
        log_d("[BTN] id %d press: %s", id, get_action_name(action));
        press_finished = true;
        state_machine_state = 0;
      }
      break;
    case 5:  // debounce next press
      if (since_press_start >= DEBOUNCE_TIMEOUT) {
        if (read_pin()) {
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
          log_d("[BTN] id %d press: %s",id, get_action_name(action));
          press_finished = true;
          release_start_time = millis();
          state_machine_state = 8;
        }
      }
      break;
    case 6:  // wait for release
      if (!read_pin()) {
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

Button::ButtonAction Button::get_action() { return action; }

bool Button::is_press_finished() { return press_finished; }

void Button::clear() { reset(); }

uint8_t Button::get_pin() { return pin; }

uint16_t Button::get_id() { return id; }

String Button::get_action_name(ButtonAction action) {
  String press;
  switch (action) {
    case IDLE:
      press = "IDLE";
      break;
    case SINGLE:
      press = "SINGLE";
      break;
    case DOUBLE:
      press = "DOUBLE";
      break;
    case TRIPLE:
      press = "TRIPLE";
      break;
    case QUAD:
      press = "QUAD";
      break;
    case LONG_1:
      press = "LONG_1";
      break;
    case LONG_2:
      press = "LONG_2";
      break;
    case LONG_3:
      press = "LONG_3";
      break;
    case LONG_4:
      press = "LONG_4";
      break;
  }
  return press;
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

void Button::isr() {
  if (read_pin()) {
    rising_flag = true;
  } else {
    falling_flag = true;
  }
}

bool Button::read_pin() {
  if (active_high) {
    return digitalRead(pin);
  } else {
    return !digitalRead(pin);
  }
}

void Button::reset() {
  rising_flag = false;
  falling_flag = false;
  press_finished = false;
  action = IDLE;
}

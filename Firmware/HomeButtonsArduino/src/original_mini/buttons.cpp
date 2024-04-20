#include "buttons.h"

#include "FunctionalInterrupt.h"

bool Button::Init(uint8_t pin, uint16_t id, bool active_high) {
  if (!(LONG_2S_TIME < LONG_5S_TIME && LONG_5S_TIME < LONG_10S_TIME &&
        LONG_10S_TIME < LONG_20S_TIME)) {
    debug("invalid press times");
    return false;
  }
  pin_ = pin;
  id_ = id;
  active_high_ = active_high;
  return UserInput::Init();
}

void Button::init_press() {
  action = ClickAction::SINGLE;
  state_machine_state = 2;
  press_start_time = millis();
}

bool Button::InternalStart() {
  attachInterrupt(pin, std::bind(&Button::ISR, this), CHANGE);
}

bool Button::InternalStop() { detachInterrupt(pin_); }

void Button::InternalLoop() {
  static uint32_t time = 0;

  switch (sm_state_) {
    case 0:  // idle
      if (rising_flag_) {
        rising_flag_ = false;
        time = millis();
        press_start_time_ = millis();
        last_trigger_time_ = millis();
        click_started_ = true;
        sm_state_ = 1;
      }
      break;
    case 1:  // debounce
      if (since_press_start >= DEBOUNCE_TIMEOUT) {
        if (ReadPin()) {
          action = ClickAction::SINGLE;
          sm_state = 2;
        } else {
          sm_state = 0;
        }
      }
      break;
    case 2:  // pin is high
      if (falling_flag || !ReadPin()) {
        falling_flag = false;
        switch (action) {
          case ClickAction::SINGLE:
            release_start_time = millis();
            sm_state = 3;
            break;
          case ClickAction::CLICK_LONG_2S:
          case ClickAction::CLICK_LONG_5S:
          case ClickAction::CLICK_LONG_10S:
          case ClickAction::CLICK_LONG_20S:
            debug("id %d press: %s", id, get_action_name(action));
            press_finished = true;
            release_start_time = millis();
            sm_state = 8;
            break;
          default:
            break;
        }
      }
      if (since_press_start >= LONG_2S_TIME &&
          since_press_start < LONG_5S_TIME) {
        action = ClickAction::CLICK_LONG_2S;
      } else if (since_press_start >= LONG_5S_TIME &&
                 since_press_start < LONG_10S_TIME) {
        action = ClickAction::CLICK_LONG_5S;
      } else if (since_press_start >= LONG_10S_TIME &&
                 since_press_start < LONG_20S_TIME) {
        action = ClickAction::CLICK_LONG_10S;
      } else if (since_press_start >= LONG_20S_TIME) {
        action = ClickAction::CLICK_LONG_20S;
      }
      break;
    case 3:  // debounce
      if (since_release_start >= DEBOUNCE_TIMEOUT) {
        sm_state = 4;
      }
      break;
    case 4:  // end of single, wait for additional presses
      if (rising_flag) {
        rising_flag = false;
        press_start_time = millis();
        sm_state = 5;
      } else if (since_press_start >= PRESS_TIMEOUT) {
        debug("id %d press: %s", id, get_action_name(action));
        press_finished = true;
        sm_state = 0;
      }
      break;
    case 5:  // debounce next press
      if (since_press_start >= DEBOUNCE_TIMEOUT) {
        if (ReadPin()) {
          switch (action) {
            case ClickAction::SINGLE:
              action = ClickAction::DOUBLE;
              break;
            case ClickAction::DOUBLE:
              action = ClickAction::TRIPLE;
              break;
            case ClickAction::TRIPLE:
              action = ClickAction::QUAD;
              break;
            default:
              break;
          }
          sm_state = 6;
        } else {
          debug("id %d press: %s", id, get_action_name(action));
          press_finished = true;
          release_start_time = millis();
          sm_state = 8;
        }
      }
      break;
    case 6:  // wait for release
      if (!ReadPin()) {
        release_start_time = millis();
        sm_state = 7;
      }
      break;
    case 7:  // debounce and return to 4
      if (since_release_start >= DEBOUNCE_TIMEOUT) {
        sm_state = 4;
      }
      break;
    case 8:
      if (since_release_start >= DEBOUNCE_TIMEOUT) {
        sm_state = 0;
      }
  }
}

uint8_t Button::pin() const { return pin_; }

uint16_t Button::id() const { return id_; }

void Button::ISR() {
  if (ReadPin()) {
    rising_flag_ = true;
  } else {
    falling_flag_ = true;
  }
}

bool Button::ReadPin() const {
  if (active_high_) {
    return digitalRead(pin_);
  } else {
    return !digitalRead(pin_);
  }
}

#include "buttons.h"

#include "FunctionalInterrupt.h"

void Button::InitPress() {
  sm_state_ = 2;
  sm_time_ = millis();
  press_start_time_ = millis();
  last_trigger_time_ = millis();
  debug("InitPress");
}

bool Button::InternalStart() {
  sm_state_ = 0;
  attachInterrupt(hw_.button_pin(id_), std::bind(&Button::ISR, this), CHANGE);
  debug("attached interrupt on pin %d", hw_.button_pin(id_));
  return true;
}

bool Button::InternalStop() {
  detachInterrupt(hw_.button_pin(id_));
  SetStopped();
  return true;
}

void Button::InternalLoop() {
  switch (sm_state_) {
    case 0:  // idle
      if (rising_flag_) {
        rising_flag_ = false;
        falling_flag_ = false;
        sm_time_ = millis();
        press_start_time_ = millis();
        last_trigger_time_ = millis();
        sm_state_ = 1;
      }
      break;
    case 1:  // debounce
      if (millis() - press_start_time_ >= kDebounceTimeout) {
        if (hw_.button_pressed(id_)) {
          sm_state_ = 2;
        } else {
          sm_state_ = 8;
        }
      }
      break;
    case 2:  // pin is high, wait for release
      if (falling_flag_ || !hw_.button_pressed(id_)) {
        falling_flag_ = false;
        num_clicks_++;
        TriggerClick(millis() - press_start_time_, num_clicks_, false);
        sm_time_ = millis();
        sm_state_ = 3;
      }
      // keep triggering click if button is held
      if (millis() - last_trigger_time_ >= kTriggerInterval) {
        last_trigger_time_ = millis();
        TriggerClick(millis() - press_start_time_, num_clicks_, false);
      }
      break;
    case 3:  // debounce
      if (millis() - sm_time_ >= kDebounceTimeout) {
        sm_state_ = 4;
      }
      break;
    case 4:  // end of single, wait for additional presses
      if (rising_flag_) {
        rising_flag_ = false;
        sm_time_ = millis();
        sm_state_ = 5;
      } else if (millis() - sm_time_ >= kPressTimeout) {
        TriggerClick(millis() - press_start_time_, num_clicks_, true);
        sm_state_ = 8;
      }
      break;
    case 5:  // debounce next press
      if (millis() - sm_time_ >= kDebounceTimeout) {
        if (hw_.button_pressed(id_)) {
          sm_state_ = 6;
        } else {
          sm_state_ = 4;
        }
      }
      break;
    case 6:  // pin is high, wait for release
      if (falling_flag_ || !hw_.button_pressed(id_)) {
        falling_flag_ = false;
        num_clicks_++;
        TriggerClick(millis() - press_start_time_, num_clicks_, false);
        sm_time_ = millis();
        sm_state_ = 7;
      }
      break;
    case 7:  // debounce and return to 4
      if (millis() - sm_time_ >= kDebounceTimeout) {
        sm_state_ = 4;
      }
      break;
    case 8:  // reset
      num_clicks_ = 0;
      sm_state_ = 0;
  }
}

void Button::InternalRestart() {
  rising_flag_ = false;
  falling_flag_ = false;
  sm_state_ = 0;
  num_clicks_ = 0;
}

void Button::TriggerClick(uint32_t duration, uint16_t btn_num_clicks_,
                          bool final) {
  debug("CLICK: duration: %d, num_clicks: %d, final: %d", duration,
        btn_num_clicks_, final);

  switch (btn_num_clicks_) {
    case 0:
      if (duration >= kLong2sTime && duration < kLong5sTime) {
        TriggerEvent(Event{EventType::kHoldLong2s, {0, 0}, id_, final});
      } else if (duration >= kLong5sTime && duration < kLong10sTime) {
        TriggerEvent(Event{EventType::kHoldLong5s, {0, 0}, id_, final});
      } else if (duration >= kLong10sTime && duration < kLong20sTime) {
        TriggerEvent(Event{EventType::kHoldLong10s, {0, 0}, id_, final});
      } else if (duration >= kLong20sTime) {
        TriggerEvent(Event{EventType::kHoldLong20s, {0, 0}, id_, final});
      }
      break;
    case 1:
      if (duration >= kLong2sTime && duration < kLong5sTime) {
        TriggerEvent(Event{EventType::kHoldLong2s, {0, 0}, id_, final});
      } else if (duration >= kLong5sTime && duration < kLong10sTime) {
        TriggerEvent(Event{EventType::kHoldLong5s, {0, 0}, id_, final});
      } else if (duration >= kLong10sTime && duration < kLong20sTime) {
        TriggerEvent(Event{EventType::kHoldLong10s, {0, 0}, id_, final});
      } else if (duration >= kLong20sTime) {
        TriggerEvent(Event{EventType::kHoldLong20s, {0, 0}, id_, final});
      } else {
        TriggerEvent(Event{EventType::kClickSingle, {0, 0}, id_, final});
      }
      break;
    case 2:
      TriggerEvent(Event{EventType::kClickDouble, {0, 0}, id_, final});
      break;
    case 3:
      TriggerEvent(Event{EventType::kClickTriple, {0, 0}, id_, final});
      break;
    case 4:
      TriggerEvent(Event{EventType::kClickQuad, {0, 0}, id_, final});
      break;
    default:
      break;
  }
}

void Button::ISR() {
  if (hw_.button_pressed(id_)) {
    rising_flag_ = true;
  } else {
    falling_flag_ = true;
  }
}

#include "touch.h"

#include "FunctionalInterrupt.h"

bool TouchInput::InternalInit() {
  touch_controller_ = new FT6X36(&Wire1, hw_.TOUCH_INT_PIN);

  touch_controller_->registerTouchHandler(std::bind(&TouchInput::TouchHandler,
                                                    this, std::placeholders::_1,
                                                    std::placeholders::_2));
  attachInterrupt(hw_.TOUCH_CLICK_PIN, std::bind(&TouchInput::ClickISR, this),
                  CHANGE);
  return true;
}

bool TouchInput::InternalStart() {
  touch_controller_->begin();
  return true;
}

bool TouchInput::InternalStop() {
  detachInterrupt(hw_.TOUCH_CLICK_PIN);
  return true;
}

void TouchInput::InternalLoop() {
  BtnUpdate();
  touch_controller_->loop();
}

void TouchInput::BtnUpdate() {
  switch (btn_sm_state_) {
    case 0:  // idle
      if (rising_flag_) {
        rising_flag_ = false;
        btn_sm_time_ = millis();
        btn_press_start_time_ = millis();
        btn_last_trigger_time_ = millis();
        click_started_ = true;
        btn_sm_state_ = 1;
      }
      break;
    case 1:  // debounce
      if (millis() - btn_press_start_time_ >= kDebounceTimeout) {
        if (ReadPin()) {
          btn_sm_state_ = 2;
        } else {
          btn_sm_state_ = 8;
        }
      }
      break;
    case 2:  // pin is high, wait for release
      if (falling_flag_ || !ReadPin()) {
        falling_flag_ = false;
        btn_num_clicks_++;
        TriggerClick(millis() - btn_press_start_time_, btn_num_clicks_, false);
        btn_sm_time_ = millis();
        btn_sm_state_ = 3;
      }
      // keep triggering click if button is held
      if (millis() - btn_last_trigger_time_ >= kTriggerInterval) {
        btn_last_trigger_time_ = millis();
        TriggerClick(millis() - btn_press_start_time_, btn_num_clicks_, false);
      }
      break;
    case 3:  // debounce
      if (millis() - btn_sm_time_ >= kDebounceTimeout) {
        btn_sm_state_ = 4;
      }
      break;
    case 4:  // end of single_wait for additional presses
      if (rising_flag_) {
        rising_flag_ = false;
        btn_sm_time_ = millis();
        btn_sm_state_ = 5;
      } else if (millis() - btn_sm_time_ >= kPressTimeout) {
        TriggerClick(millis() - btn_press_start_time_, btn_num_clicks_, true);
        btn_sm_state_ = 8;
      }
      break;
    case 5:  // debounce next press
      if (millis() - btn_sm_time_ >= kDebounceTimeout) {
        if (ReadPin()) {
          btn_sm_state_ = 6;
        } else {
          btn_sm_state_ = 4;
        }
      }
      break;
    case 6:  // pin is high, wait for release
      if (falling_flag_ || !ReadPin()) {
        falling_flag_ = false;
        btn_num_clicks_++;
        TriggerClick(millis() - btn_press_start_time_, btn_num_clicks_, false);
        btn_sm_time_ = millis();
        btn_sm_state_ = 7;
      }
      break;
    case 7:  // debounce and return to 4
      if (millis() - btn_sm_time_ >= kDebounceTimeout) {
        btn_sm_state_ = 4;
      }
      break;
    case 8:  // reset
      click_started_ = false;
      btn_num_clicks_ = 0;
      btn_sm_state_ = 0;
  }
}

void TouchInput::TouchHandler(TPoint point, TEvent e) {
  last_touch_point_ = {point.x, point.y};
  switch (e) {
    case TEvent::TouchStart:
      debug("touch start %d %d", point.x, point.y);
      touch_started_ = true;
      touch_start_point_ = last_touch_point_;
      touch_start_time_ = millis();
      TriggerTouch(TEvent::TouchStart, last_touch_point_);
      break;
    case TEvent::TouchMove:
      debug("touch move %d %d", point.x, point.y);
      break;
    case TEvent::TouchEnd:
      debug("touch end %d %d", point.x, point.y);
      TriggerTouch(TEvent::TouchEnd, last_touch_point_);
      touch_started_ = false;
      break;
    case TEvent::Tap:
      debug("tap %d %d", point.x, point.y);
      TriggerTouch(TEvent::Tap, last_touch_point_);
      break;
    case TEvent::DragStart:
      debug("drag start %d %d", point.x, point.y);
      break;
    case TEvent::DragMove:
      debug("drag move %d %d", point.x, point.y);
      break;
    case TEvent::DragEnd:
      debug("drag end %d %d", point.x, point.y);
      break;
    default:
      debug("unknown touch event %d", static_cast<int>(e));
      break;
  }
}

void IRAM_ATTR TouchInput::ClickISR() {
  rising_flag_ = digitalRead(click_pin_);
  falling_flag_ = !digitalRead(click_pin_);
}

bool TouchInput::ReadPin() const {
  if (active_high_) {
    return digitalRead(click_pin_);
  } else {
    return !digitalRead(click_pin_);
  }
}

void TouchInput::TriggerClick(uint32_t duration, uint16_t btn_num_clicks_,
                              bool finished) {
  debug("CLICK: duration: %d, num_clicks: %d, finished: %d", duration,
        btn_num_clicks_, finished);
  uint8_t btn_num = Touch2BtnNum(last_touch_point_);

  switch (btn_num_clicks_) {
    case 0:
      if (duration >= kLong2sTime && duration < kLong5sTime) {
        TriggerEvent(Event{EventType::kHoldLong2s, last_touch_point_, btn_num});
      } else if (duration >= kLong5sTime && duration < kLong10sTime) {
        TriggerEvent(Event{EventType::kHoldLong5s, last_touch_point_, btn_num});
      } else if (duration >= kLong10sTime && duration < kLong20sTime) {
        TriggerEvent(
            Event{EventType::kHoldLong10s, last_touch_point_, btn_num});
      } else if (duration >= kLong20sTime) {
        TriggerEvent(
            Event{EventType::kHoldLong20s, last_touch_point_, btn_num});
      }
      break;
    case 1:
      if (duration >= kLong2sTime && duration < kLong5sTime) {
        TriggerEvent(
            Event{EventType::kClickLong2s, last_touch_point_, btn_num});
      } else if (duration >= kLong5sTime && duration < kLong10sTime) {
        TriggerEvent(
            Event{EventType::kClickLong5s, last_touch_point_, btn_num});
      } else if (duration >= kLong10sTime && duration < kLong20sTime) {
        TriggerEvent(
            Event{EventType::kClickLong10s, last_touch_point_, btn_num});
      } else if (duration >= kLong20sTime) {
        TriggerEvent(
            Event{EventType::kClickLong20s, last_touch_point_, btn_num});
      } else {
        TriggerEvent(
            Event{EventType::kClickSingle, last_touch_point_, btn_num});
      }
      break;
    case 2:
      TriggerEvent(Event{EventType::kClickDouble, last_touch_point_, btn_num});
      break;
    case 3:
      TriggerEvent(Event{EventType::kClickTriple, last_touch_point_, btn_num});
      break;
    case 4:
      TriggerEvent(Event{EventType::kClickQuad, last_touch_point_, btn_num});
      break;
    default:
      break;
  }
}

void TouchInput::TriggerTouch(TEvent event, TouchPoint point) {
  uint8_t btn_num = Touch2BtnNum(last_touch_point_);
  switch (event) {
    case TEvent::TouchStart:
      TriggerEvent(Event{EventType::kTouchStart, point, btn_num});
      break;
    case TEvent::Tap:
      if (!click_started_) {
        TriggerEvent(Event{EventType::kTap, point, btn_num});
      }
      break;
    case TEvent::TouchEnd:
      if (touch_started_) {
        if (millis() - touch_start_time_ <= kSwipeTimeout) {
          if (abs(point.x - touch_start_point_.x) >= kSwipeDistance) {
            if (point.x > touch_start_point_.x) {
              TriggerEvent(Event{EventType::kSwipeRight, point, btn_num});
            } else {
              TriggerEvent(Event{EventType::kSwipeLeft, point, btn_num});
            }
          } else if (abs(point.y - touch_start_point_.y) >= kSwipeDistance) {
            if (point.y > touch_start_point_.y) {
              TriggerEvent(Event{EventType::kSwipeDown, point, btn_num});
            } else {
              TriggerEvent(Event{EventType::kSwipeUp, point, btn_num});
            }
          }
        }
      }
      TriggerEvent(Event{EventType::kTouchEnd, point, btn_num});
      break;
    default:
      break;
  }
}

uint8_t TouchInput::Touch2BtnNum(TouchPoint point) {
  // 9 buttons on 400x300 touchscreen
  if (point.x < 134) {
    if (point.y < 100) {
      return 1;
    } else if (point.y < 200) {
      return 4;
    } else {
      return 7;
    }
  } else if (point.x < 266) {
    if (point.y < 100) {
      return 2;
    } else if (point.y < 200) {
      return 5;
    } else {
      return 8;
    }
  } else {
    if (point.y < 100) {
      return 3;
    } else if (point.y < 200) {
      return 6;
    } else {
      return 9;
    }
  }
}
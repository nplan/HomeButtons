#include "touch.h"

#include "FunctionalInterrupt.h"

bool TouchInput::Init(uint8_t click_pin, uint8_t int_pin, bool active_high) {
  touch_controller_ = new FT6X36(&Wire1, int_pin);
  click_pin_ = click_pin;
  active_high_ = active_high;
  touch_controller_->registerTouchHandler(std::bind(&TouchInput::touch_handler,
                                                    this, std::placeholders::_1,
                                                    std::placeholders::_2));
  attachInterrupt(click_pin_, std::bind(&TouchInput::_click_isr, this), CHANGE);
  return UserInput::Init();
}

bool TouchInput::InternalStart() {
  touch_controller_->begin();
  return true;
}

bool TouchInput::InternalStop() {
  detachInterrupt(click_pin_);
  return true;
}

void TouchInput::InternalLoop() {
  _btn_update();
  touch_controller_->loop();
}

void TouchInput::_btn_update() {
  static uint32_t time = 0;
  static uint8_t sm_state = 0;
  static uint8_t num_clicks = 0;
  static uint32_t last_trigger_time = 0;
  static uint32_t press_start_time = 0;

  switch (sm_state) {
    case 0:  // idle
      if (rising_flag_) {
        rising_flag_ = false;
        time = millis();
        press_start_time = millis();
        last_trigger_time = millis();
        click_started_ = true;
        sm_state = 1;
      }
      break;
    case 1:  // debounce
      if (millis() - press_start_time >= kDebounceTimeout) {
        if (_read_pin()) {
          sm_state = 2;
          debug("BTN CLICK");
        } else {
          sm_state = 8;
        }
      }
      break;
    case 2:  // pin is high, wait for release
      if (falling_flag_ || !_read_pin()) {
        falling_flag_ = false;
        num_clicks++;
        _trigger_click(millis() - press_start_time, num_clicks, false);
        time = millis();
        sm_state = 3;
      }
      if (millis() - last_trigger_time >= kTriggerInterval) {
        last_trigger_time = millis();
        _trigger_click(millis() - press_start_time, num_clicks, false);
      }
      break;
    case 3:  // debounce
      if (millis() - time >= kDebounceTimeout) {
        sm_state = 4;
      }
      break;
    case 4:  // end of single_wait for additional
      if (rising_flag_) {
        rising_flag_ = false;
        time = millis();
        sm_state = 5;
      } else if (millis() - time >= kPressTimeout) {
        _trigger_click(millis() - press_start_time, num_clicks, true);
        sm_state = 8;
      }
      break;
    case 5:  // debounce next press
      if (millis() - time >= kDebounceTimeout) {
        if (_read_pin()) {
          sm_state = 6;
        } else {
          sm_state = 4;
        }
      }
      break;
    case 6:  // pin is high, wait for release
      if (falling_flag_ || !_read_pin()) {
        falling_flag_ = false;
        num_clicks++;
        _trigger_click(millis() - press_start_time, num_clicks, false);
        time = millis();
        sm_state = 7;
      }
      break;
    case 7:  // debounce and return to 4
      if (millis() - time >= kDebounceTimeout) {
        sm_state = 4;
      }
      break;
    case 8:  // reset
      click_started_ = false;
      num_clicks = 0;
      sm_state = 0;
  }
}

void TouchInput::touch_handler(TPoint point, TEvent e) {
  last_touch_point_ = {point.x, point.y};
  switch (e) {
    case TEvent::TouchStart:
      debug("touch start %d %d", point.x, point.y);
      touch_started_ = true;
      touch_start_point_ = last_touch_point_;
      touch_start_time_ = millis();
      _trigger_touch(TEvent::TouchStart, last_touch_point_);
      break;
    case TEvent::TouchMove:
      debug("touch move %d %d", point.x, point.y);
      break;
    case TEvent::TouchEnd:
      debug("touch end %d %d", point.x, point.y);
      _trigger_touch(TEvent::TouchEnd, last_touch_point_);
      touch_started_ = false;
      break;
    case TEvent::Tap:
      debug("tap %d %d", point.x, point.y);
      _trigger_touch(TEvent::Tap, last_touch_point_);
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

void IRAM_ATTR TouchInput::_click_isr() {
  rising_flag_ = digitalRead(click_pin_);
  falling_flag_ = !digitalRead(click_pin_);
}

bool TouchInput::_read_pin() const {
  if (active_high_) {
    return digitalRead(click_pin_);
  } else {
    return !digitalRead(click_pin_);
  }
}

void TouchInput::_trigger_event(Event event) {
  info("UI event: %s, x: %d, y: %d", EventType2Str(event.type), event.point.x,
       event.point.y);
  if (event_callback_) {
    event_callback_(event);
  }
  if (event_callback_2_) {
    event_callback_2_(event);
  }
}

void TouchInput::_trigger_click(uint32_t duration, uint16_t num_clicks,
                                bool finished) {
  debug("CLICK: duration: %d, num_clicks: %d, finished: %d", duration,
        num_clicks, finished);
  uint8_t btn_num = Touch2BtnNum(last_touch_point_);

  switch (num_clicks) {
    case 0:
      if (duration >= kLong2sTime && duration < kLong5sTime) {
        _trigger_event(
            Event{EventType::kHoldLong2s, last_touch_point_, btn_num});
      } else if (duration >= kLong5sTime && duration < kLong10sTime) {
        _trigger_event(
            Event{EventType::kHoldLong5s, last_touch_point_, btn_num});
      } else if (duration >= kLong10sTime && duration < kLong20sTime) {
        _trigger_event(
            Event{EventType::kHoldLong10s, last_touch_point_, btn_num});
      } else if (duration >= kLong20sTime) {
        _trigger_event(
            Event{EventType::kHoldLong20s, last_touch_point_, btn_num});
      }
      break;
    case 1:
      if (duration >= kLong2sTime && duration < kLong5sTime) {
        _trigger_event(
            Event{EventType::kClickLong2s, last_touch_point_, btn_num});
      } else if (duration >= kLong5sTime && duration < kLong10sTime) {
        _trigger_event(
            Event{EventType::kClickLong5s, last_touch_point_, btn_num});
      } else if (duration >= kLong10sTime && duration < kLong20sTime) {
        _trigger_event(
            Event{EventType::kClickLong10s, last_touch_point_, btn_num});
      } else if (duration >= kLong20sTime) {
        _trigger_event(
            Event{EventType::kClickLong20s, last_touch_point_, btn_num});
      } else {
        _trigger_event(
            Event{EventType::kClickSingle, last_touch_point_, btn_num});
      }
      break;
    case 2:
      _trigger_event(
          Event{EventType::kClickDouble, last_touch_point_, btn_num});
      break;
    case 3:
      _trigger_event(
          Event{EventType::kClickTriple, last_touch_point_, btn_num});
      break;
    case 4:
      _trigger_event(Event{EventType::kClickQuad, last_touch_point_, btn_num});
      break;
    default:
      break;
  }
}

void TouchInput::_trigger_touch(TEvent event, TouchPoint point) {
  uint8_t btn_num = Touch2BtnNum(last_touch_point_);
  switch (event) {
    case TEvent::TouchStart:
      _trigger_event(Event{EventType::kTouchStart, point, btn_num});
      break;
    case TEvent::Tap:
      if (!click_started_) {
        _trigger_event(Event{EventType::kTap, point, btn_num});
      }
      break;
    case TEvent::TouchEnd:
      if (touch_started_) {
        if (millis() - touch_start_time_ <= kSwipeTimeout) {
          if (abs(point.x - touch_start_point_.x) >= kSwipeDistance) {
            if (point.x > touch_start_point_.x) {
              _trigger_event(Event{EventType::kSwipeRight, point, btn_num});
            } else {
              _trigger_event(Event{EventType::kSwipeLeft, point, btn_num});
            }
          } else if (abs(point.y - touch_start_point_.y) >= kSwipeDistance) {
            if (point.y > touch_start_point_.y) {
              _trigger_event(Event{EventType::kSwipeDown, point, btn_num});
            } else {
              _trigger_event(Event{EventType::kSwipeUp, point, btn_num});
            }
          }
        }
      }
      _trigger_event(Event{EventType::kTouchEnd, point, btn_num});
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
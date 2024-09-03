#include "btn_sw_led.h"
#include "FunctionalInterrupt.h"

void BtnSwLEDStates::IdleState::entry() {
  sm().rising_flag_ = false;
  sm().falling_flag_ = false;
  sm().num_clicks_ = 0;
  if (sm().switch_mode_ && sm().is_kill_switch_) {
    if (sm().hw_.button_pressed(sm().id_)) {
      return transition_to<SwOnState>();
    } else {
      return transition_to<SwOffState>();
    }
  }
}

void BtnSwLEDStates::IdleState::loop() {
  if (sm().rising_flag_) {
    return transition_to<RisingDebounceState>();
  }
}

void BtnSwLEDStates::RisingDebounceState::entry() {
  sm().rising_flag_ = false;
  sm().falling_flag_ = false;
  start_time_ = millis();
  sm().press_start_time_ = millis();
}

void BtnSwLEDStates::RisingDebounceState::loop() {
  if (millis() - start_time_ >= kBtnDebounceTimeout) {
    if (sm().hw_.button_pressed(sm().id_)) {
      if (sm().switch_mode_ && sm().is_kill_switch_) {
        sm().TriggerSwitch(true);
        return transition_to<SwOnState>();
      } else {
        return transition_to<PressedState>();
      }
    } else {
      if (sm().switch_mode_) {
        if (sm().switch_state_) {
          return transition_to<SwOnState>();
        } else {
          return transition_to<SwOffState>();
        }
      } else {
        return transition_to<IdleState>();
      }
    }
  }
}

void BtnSwLEDStates::PressedState::entry() {
  sm().rising_flag_ = false;
  sm().falling_flag_ = false;
  last_trigger_time_ = millis();
}

void BtnSwLEDStates::PressedState::loop() {
  if (sm().falling_flag_) {
    if (sm().switch_mode_) {
      if (!sm().is_kill_switch_) {
        if (sm().switch_state_) {
          sm().TriggerSwitch(false);
          return transition_to<SwOffState>();
        } else {
          sm().TriggerSwitch(true);
          return transition_to<SwOnState>();
        }
      } else {
        sm().TriggerSwitch(true);
        return transition_to<SwOnState>();
      }
    } else {
      sm().num_clicks_++;
      sm().TriggerClick(millis() - sm().press_start_time_, sm().num_clicks_,
                        false);
      return transition_to<FallingDebounceState>();
    }
  }
  if (!sm().switch_mode_ || !sm().is_kill_switch_) {
    // keep triggering click if button is held
    if (millis() - last_trigger_time_ >= kBtnTriggerInterval) {
      last_trigger_time_ = millis();
      sm().TriggerClick(millis() - sm().press_start_time_, sm().num_clicks_,
                        false);
    }
  }
}

void BtnSwLEDStates::FallingDebounceState::entry() {
  sm().rising_flag_ = false;
  sm().falling_flag_ = false;
  start_time_ = millis();
}

void BtnSwLEDStates::FallingDebounceState::loop() {
  if (millis() - start_time_ >= kBtnDebounceTimeout) {
    if (sm().switch_mode_) {
      sm().TriggerSwitch(false);
      return transition_to<SwOffState>();
    } else {
      return transition_to<ReleasedState>();
    }
  }
}

void BtnSwLEDStates::ReleasedState::entry() {
  sm().rising_flag_ = false;
  sm().falling_flag_ = false;
  start_time_ = millis();
}

void BtnSwLEDStates::ReleasedState::loop() {
  if (sm().rising_flag_) {
    return transition_to<RisingDebounceState>();
  }
  if (millis() - start_time_ >= kBtnPressTimeout) {
    sm().TriggerClick(millis() - sm().press_start_time_, sm().num_clicks_,
                      true);
    return transition_to<IdleState>();
  }
}

void BtnSwLEDStates::SwOnState::entry() {
  sm().rising_flag_ = false;
  sm().falling_flag_ = false;
  sm().switch_state_ = true;
  sm().LEDOn(sm().hw_.LED_BRIGHT_DFLT);
}

void BtnSwLEDStates::SwOnState::loop() {
  if (!sm().is_kill_switch_) {
    if (sm().rising_flag_) {
      return transition_to<RisingDebounceState>();
    }
  } else {
    if (sm().falling_flag_) {
      return transition_to<FallingDebounceState>();
    }
  }
}

void BtnSwLEDStates::SwOffState::entry() {
  sm().rising_flag_ = false;
  sm().falling_flag_ = false;
  sm().switch_state_ = false;
  sm().LEDOff();
}

void BtnSwLEDStates::SwOffState::loop() {
  if (sm().rising_flag_) {
    return transition_to<RisingDebounceState>();
  }
}

void BtnSwLED::InitPress() {
  press_start_time_ = millis();
  debug("init press set");
  transition_to<BtnSwLEDStates::PressedState>();
}

bool BtnSwLED::InternalStart() {
  led_.Start();
  attachInterrupt(hw_.button_pin(id_), std::bind(&BtnSwLED::ISR, this), CHANGE);
  debug("attached interrupt on pin %d", hw_.button_pin(id_));
  return true;
}

bool BtnSwLED::InternalStop() {
  led_.Stop();
  detachInterrupt(hw_.button_pin(id_));
  SetStopped();
  return true;
}

void BtnSwLED::TriggerClick(uint32_t duration, uint16_t btn_num_clicks_,
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

void BtnSwLED::TriggerSwitch(bool on) {
  if (on) {
    TriggerEvent(Event{EventType::kSwitchOn, {0, 0}, id_, true});
    debug("Switch %d ON", id_);
  } else {
    TriggerEvent(Event{EventType::kSwitchOff, {0, 0}, id_, true});
    debug("Switch %d OFF", id_);
  }
}

void BtnSwLED::ISR() {
  if (hw_.button_pressed(id_)) {
    rising_flag_ = true;
  } else {
    falling_flag_ = true;
  }
}
#include "leds.h"

void LEDSMStates::IdleState::entry() {
  sm().hw_.set_led_num(sm().id_, sm().ambient_brightness_);
}

void LEDSMStates::IdleState::loop() {
  if (sm().cmd_blink_.has_value()) {
    sm().current_blink_ = sm().cmd_blink_;
    sm().cmd_blink_ = std::nullopt;
    sm().current_blink_num_ = 0;

    if (sm().current_blink_->type == LEDBlinkType::kBlink) {
      return transition_to<BlinkOnState>();
    } else if (sm().current_blink_->type == LEDBlinkType::kConstant) {
      return transition_to<ConstOnState>();
    } else if (sm().current_blink_->type == LEDBlinkType::kPulse) {
      return transition_to<PulseState>();
    } else if (sm().current_blink_->type == LEDBlinkType::kOff) {
      return transition_to<IdleState>();
    }
  }
  if (sm().cstate() == ComponentBase::ComponentState::kCmdStop) {
    sm().SetStopped();
  }
}

void LEDSMStates::BlinkOnState::entry() {
  sm().hw_.set_led_num(sm().id_, sm().current_blink_->brightness);
  sm().last_change_time_ = millis();
}

void LEDSMStates::BlinkOnState::loop() {
  if (millis() - sm().last_change_time_ >= sm().current_blink_->on_ms) {
    if (sm().current_blink_->hold &&
        sm().current_blink_num_ == sm().current_blink_->num_blinks - 1) {
      return transition_to<IdleState>();
    } else {
      return transition_to<BlinkOffState>();
    }
  }
}

void LEDSMStates::BlinkOffState::entry() {
  sm().hw_.set_led_num(sm().id_, sm().ambient_brightness_);
  sm().last_change_time_ = millis();
}

void LEDSMStates::BlinkOffState::loop() {
  if (millis() - sm().last_change_time_ >= sm().current_blink_->off_ms) {
    if (sm().current_blink_num_ < sm().current_blink_->num_blinks - 1) {
      sm().current_blink_num_++;
      return transition_to<BlinkOnState>();
    } else {
      return transition_to<IdleState>();
    }
  }
}

void LEDSMStates::ConstOnState::entry() {
  sm().hw_.set_led_num(sm().id_, sm().current_blink_->brightness);
}

void LEDSMStates::ConstOnState::loop() {
  if (sm().cmd_blink_.has_value()) {
    return transition_to<IdleState>();
  }
  if (sm().cstate() == ComponentBase::ComponentState::kCmdStop) {
    sm().SetStopped();
  }
}

void LEDSMStates::PulseState::entry() { sm().last_change_time_ = millis(); }

void LEDSMStates::PulseState::loop() {
  uint8_t b = sm().current_blink_->brightness *
              (-0.5 * cos(2 * PI * (millis() - sm().last_change_time_) /
                          sm().current_blink_->cycle_ms) +
               0.5);
  sm().hw_.set_led_num(sm().id_, b);
  if (sm().cmd_blink_.has_value()) {
    return transition_to<IdleState>();
  }
  if (sm().cstate() == ComponentBase::ComponentState::kCmdStop) {
    sm().SetStopped();
  }
}

void LEDSMStates::TransitionState::entry() {
  start_time_ = millis();
  sm().hw_.set_led_num(sm().id_, sm().ambient_brightness_);
}

void LEDSMStates::TransitionState::loop() {
  if (millis() - start_time_ >= 1000) {
    return transition_to<IdleState>();
  }
}

void LED::Blink(uint8_t num_blinks, uint16_t brightness, uint16_t on_ms,
                uint16_t off_ms, bool hold) {
  if (on_ms == 0) {
    switch (num_blinks) {
      case 1:
        on_ms = 400;
        break;
      case 2:
        on_ms = 100;
        break;
      case 3:
        on_ms = 67;
        break;
      case 4:
        on_ms = 50;
        break;
      default:
        on_ms = 50;
        break;
    }
  }
  if (off_ms == 0) {
    switch (num_blinks) {
      case 1:
        off_ms = 400;
        break;
      case 2:
        off_ms = 300;
        break;
      case 3:
        off_ms = 200;
        break;
      case 4:
        off_ms = 150;
        break;
      default:
        off_ms = 150;
        break;
    }
  }
  cmd_blink_ = LEDBlink{
      LEDBlinkType::kBlink, brightness, num_blinks, on_ms, off_ms, hold};
  debug("BLINK: led: %d, blinks: %d, bri: %d, on_ms: %d, off_ms: %d, hold: %d",
        id_, num_blinks, brightness, on_ms, off_ms, hold);
}

void LED::On(uint16_t brightness) {
  cmd_blink_ = LEDBlink{LEDBlinkType::kConstant, brightness};
  debug("ON: led: %d, bri: %d", id_, brightness);
}

void LED::Off() {
  cmd_blink_ = LEDBlink{LEDBlinkType::kOff};
  debug("OFF: led: %d", id_);
}

void LED::Pulse(uint16_t brightness, uint16_t cycle_ms) {
  cmd_blink_ =
      LEDBlink{LEDBlinkType::kPulse, brightness, 0, 0, 0, false, cycle_ms};
  debug("PULSE: led: %d, bri: %d, cycle_ms: %d", id_, brightness, cycle_ms);
}

bool LED::InternalStop() { return true; }

void LED::InternalLoop() { LEDStateMachine::loop(); }

void LED::InternalRestart() {
  cmd_blink_ = std::nullopt;
  current_blink_ = std::nullopt;
  current_blink_num_ = 0;
  transition_to<LEDSMStates::IdleState>();
}

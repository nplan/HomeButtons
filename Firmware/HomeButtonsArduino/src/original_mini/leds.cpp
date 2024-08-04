#include "leds.h"

void LEDSMStates::IdleState::loop() {
  if (sm().cmd_blink_.has_value()) {
    sm().current_blink_ = sm().cmd_blink_;
    sm().cmd_blink_ = std::nullopt;
    sm().current_blink_num_ = 0;
    return transition_to<OnState>();
  }
  if (sm().cstate() == ComponentBase::ComponentState::kCmdStop) {
    sm().SetStopped();
  }
}

void LEDSMStates::OnState::entry() {
  sm().hw_.set_led_num(sm().id_, sm().current_blink_->brightness);
  sm().last_change_time_ = millis();
}

void LEDSMStates::OnState::loop() {
  if (millis() - sm().last_change_time_ >= sm().current_blink_->on_ms) {
    if (sm().current_blink_->hold &&
        sm().current_blink_num_ == sm().current_blink_->num_blinks - 1) {
      return transition_to<IdleState>();
    } else {
      return transition_to<OffState>();
    }
  }
}

void LEDSMStates::OffState::entry() {
  sm().hw_.set_led_num(sm().id_, 0);
  sm().last_change_time_ = millis();
}

void LEDSMStates::OffState::loop() {
  if (millis() - sm().last_change_time_ >= sm().current_blink_->off_ms) {
    if (sm().current_blink_num_ < sm().current_blink_->num_blinks - 1) {
      sm().current_blink_num_++;
      return transition_to<OnState>();
    } else {
      return transition_to<IdleState>();
    }
  }
}

void LED::Blink(uint8_t num_blinks, uint8_t brightness, uint16_t on_ms,
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
    cmd_blink_ = LEDBlink{num_blinks, brightness, on_ms, off_ms, hold};
    debug(
        "blink - led: %d, blinks: %d, bri: %d, on_ms: %d, off_ms: %d, hold: %d",
        id_, num_blinks, brightness, on_ms, off_ms, hold);
  }
}

bool LED::InternalStop() { return true; }

void LED::InternalLoop() { LEDStateMachine::loop(); }

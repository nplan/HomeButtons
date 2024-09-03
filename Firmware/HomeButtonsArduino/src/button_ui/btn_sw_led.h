#ifndef HOMEBUTTONS_BLS_H
#define HOMEBUTTONS_BLS_H

#include "types.h"
#include "user_input.h"
#include "hardware.h"
#include "state_machine.h"
#include "leds.h"

class BtnSwLED;

namespace BtnSwLEDStates {

class IdleState : public State<BtnSwLED> {
 public:
  using State<BtnSwLED>::State;

  void entry() override;
  void loop() override;

  const char* get_name() override { return "IdleState"; }
};

class RisingDebounceState : public State<BtnSwLED> {
 public:
  using State<BtnSwLED>::State;

  void entry() override;
  void loop() override;

  const char* get_name() override { return "RisingDebounceState"; }

 private:
  uint32_t start_time_ = 0;
};

class PressedState : public State<BtnSwLED> {
 public:
  using State<BtnSwLED>::State;

  void entry() override;
  void loop() override;

  const char* get_name() override { return "PressedState"; }

 private:
  uint32_t last_trigger_time_ = 0;
};

class FallingDebounceState : public State<BtnSwLED> {
 public:
  using State<BtnSwLED>::State;

  void entry() override;
  void loop() override;

  const char* get_name() override { return "FallingDebounceState"; }

 private:
  uint32_t start_time_ = 0;
};

class ReleasedState : public State<BtnSwLED> {
 public:
  using State<BtnSwLED>::State;

  void entry() override;
  void loop() override;

  const char* get_name() override { return "ReleasedState"; }

 private:
  uint32_t start_time_ = 0;
};

class SwOnState : public State<BtnSwLED> {
 public:
  using State<BtnSwLED>::State;

  void entry() override;
  void loop() override;

  const char* get_name() override { return "SwOnState"; }
};

class SwOffState : public State<BtnSwLED> {
 public:
  using State<BtnSwLED>::State;

  void entry() override;
  void loop() override;

  const char* get_name() override { return "SwOffState"; }
};

}  // namespace BtnSwLEDStates

using BtnSwLEDStateMachine = StateMachine<
    BtnSwLED, BtnSwLEDStates::IdleState, BtnSwLEDStates::RisingDebounceState,
    BtnSwLEDStates::PressedState, BtnSwLEDStates::FallingDebounceState,
    BtnSwLEDStates::ReleasedState, BtnSwLEDStates::SwOnState,
    BtnSwLEDStates::SwOffState>;

class BtnSwLED : public UserInput, public BtnSwLEDStateMachine {
 public:
  BtnSwLED(const char* name, uint16_t id, bool is_kill_switch_, bool has_led,
           HardwareDefinition& hw)
      : UserInput(name),
        BtnSwLEDStateMachine(name, *this),
        id_(id),
        hw_(hw),
        is_kill_switch_(is_kill_switch_),
        has_led_(has_led),
        led_(name, id, hw) {}

  uint16_t id() const { return id_; }
  uint8_t pin() const { return hw_.button_pin(id_); }
  bool switch_state() const { return switch_state_; }

  void InitPress();

  bool switch_mode() const { return switch_mode_; }
  bool is_kill_switch() const { return is_kill_switch_; }

  void SetSwitchMode(bool on) {
    debug("set switch mode: %d", on);
    switch_mode_ = on;
    Restart();
  }

  void PauseSwitchMode() {
    saved_switch_mode_ = switch_mode_;
    saved_switch_state_ = switch_state_;
    debug("pause switch mode: %d, switch_state: %d", saved_switch_mode_,
          saved_switch_state_);
    switch_mode_ = false;
    switch_state_ = false;
    transition_to<BtnSwLEDStates::IdleState>();
  }

  void ResumeSwitchMode() {
    switch_mode_ = saved_switch_mode_;
    switch_state_ = saved_switch_state_;
    debug("resume switch mode: %d, switch_state: %d", switch_mode_,
          switch_state_);
    if (switch_mode_) {
      if (switch_state_) {
        transition_to<BtnSwLEDStates::SwOnState>();
      } else {
        transition_to<BtnSwLEDStates::SwOffState>();
      }
    } else {
      transition_to<BtnSwLEDStates::IdleState>();
    }
  }

  void SetSwitchOn() {
    debug("set switch on");
    transition_to<BtnSwLEDStates::SwOnState>();
  }

  void SetSwitchOff() {
    debug("set switch off");
    transition_to<BtnSwLEDStates::SwOffState>();
  }

  void SetAutoLED(bool on) {
    debug("set auto led: %d", on);
    auto_led_ = on;
  }

  void LEDOn(uint16_t brightness) {
    if (has_led_) {
      led_.On(brightness);
    }
  }

  void LEDOff() {
    if (has_led_) {
      led_.Off();
    }
  }

  void LEDBlink(uint8_t num_blinks, uint16_t brightness, uint16_t on_ms = 0,
                uint16_t off_ms = 0, bool hold = false) {
    if (has_led_) {
      led_.Blink(num_blinks, brightness, on_ms, off_ms, hold);
    }
  }

  void LEDPulse(uint16_t brightness, uint16_t cycle_ms = 1000) {
    if (has_led_) {
      led_.Pulse(brightness, cycle_ms);
    }
  }

  void LEDSetAmbientBrightness(uint16_t brightness) {
    if (has_led_) {
      led_.SetAmbientBrightness(brightness);
    }
  }

 private:
  bool InternalInit() override {
    led_.Init();
    return true;
  }
  bool InternalStart() override;
  bool InternalStop() override;
  void InternalLoop() override {
    BtnSwLEDStateMachine::loop();
    if (has_led_) {
      led_.Loop();
    }
  }
  void InternalRestart() override {
    switch_state_ = false;
    if (has_led_) {
      led_.Restart();
    }
    transition_to<BtnSwLEDStates::IdleState>();
  }

  void TriggerClick(uint32_t duration, uint16_t num_clicks,
                    bool finished = true);
  void TriggerSwitch(bool on);

  uint16_t id_ = 0;
  HardwareDefinition& hw_;

  bool is_kill_switch_ = false;
  bool has_led_ = false;
  LED led_;

  uint32_t press_start_time_ = 0;
  uint8_t num_clicks_ = 0;
  // uint32_t last_trigger_time_ = 0;

  bool switch_mode_ = false;
  bool switch_state_ = false;
  bool saved_switch_mode_ = false;
  bool saved_switch_state_ = false;

  bool rising_flag_ = false;
  bool falling_flag_ = false;

  bool auto_led_ = false;

  void IRAM_ATTR ISR();

  friend class BtnSwLEDStates::IdleState;
  friend class BtnSwLEDStates::RisingDebounceState;
  friend class BtnSwLEDStates::PressedState;
  friend class BtnSwLEDStates::FallingDebounceState;
  friend class BtnSwLEDStates::ReleasedState;
  friend class BtnSwLEDStates::SwOnState;
  friend class BtnSwLEDStates::SwOffState;
};

template <uint8_t N>
class BtnSwLEDInput : public UserInput {
 public:
  BtnSwLEDInput(const char* name,
                std::array<std::reference_wrapper<BtnSwLED>, N> bls)
      : UserInput(name), bls_(bls) {}

  uint16_t IdFromPin(uint8_t pin) {
    for (auto& bls : bls_) {
      if (bls.get().pin() == pin) {
        return bls.get().id();
      }
    }
    return 0;
  }

  std::optional<std::reference_wrapper<BtnSwLED>> GetBtnSwLED(uint8_t bls_id) {
    for (auto& bls : bls_) {
      if (bls.get().id() == bls_id) {
        std::optional<std::reference_wrapper<BtnSwLED>> ret = bls.get();
        return ret;
      }
    }
    return std::nullopt;
  }

  std::array<std::reference_wrapper<BtnSwLED>, N> GetBtnSwLEDs() {
    return bls_;
  }

  void InitPress(uint8_t bls_id) {
    auto bls = GetBtnSwLED(bls_id);
    if (bls) {
      bls.value().get().InitPress();
    }
  }

  void SetSwitchMode(uint8_t bls_id, bool on) {
    auto bls = GetBtnSwLED(bls_id);
    if (bls) {
      bls.value().get().SetSwitchMode(on);
    }
  }

  void SetSwitchMode(bool on) {
    for (auto& bls : bls_) {
      bls.get().SetSwitchMode(on);
    }
  }

  void PauseSwitchMode(uint8_t bls_id) {
    auto bls = GetBtnSwLED(bls_id);
    if (bls) {
      bls.value().get().PauseSwitchMode();
    }
  }

  void PauseSwitchMode() {
    for (auto& bls : bls_) {
      bls.get().PauseSwitchMode();
    }
  }

  void ResumeSwitchMode(uint8_t bls_id) {
    auto bls = GetBtnSwLED(bls_id);
    if (bls) {
      bls.value().get().ResumeSwitchMode();
    }
  }

  void ResumeSwitchMode() {
    for (auto& bls : bls_) {
      bls.get().ResumeSwitchMode();
    }
  }

  void SetAutoLED(uint8_t bls_id, bool on) {
    auto bls = GetBtnSwLED(bls_id);
    if (bls) {
      bls.value().get().SetAutoLED(on);
    }
  }

  void SetAutoLED(bool on) {
    for (auto& bls : bls_) {
      bls.get().SetAutoLED(on);
    }
  }

  bool SwitchState(uint8_t bls_id) {
    auto bls = GetBtnSwLED(bls_id);
    if (bls) {
      return bls.value().get().switch_state();
    }
  }

  void LEDOn(uint8_t bls_id, uint16_t brightness) {
    auto bls = GetBtnSwLED(bls_id);
    if (bls) {
      bls.value().get().LEDOn(brightness);
    }
  }

  void LEDOn(uint16_t brightness) {
    for (auto& bls : bls_) {
      bls.get().LEDOn(brightness);
    }
  }

  void LEDOff(uint8_t bls_id) {
    auto bls = GetBtnSwLED(bls_id);
    if (bls) {
      bls.value().get().LEDOff();
    }
  }

  void LEDOff() {
    for (auto& bls : bls_) {
      bls.get().LEDOff();
    }
  }

  void LEDBlink(uint8_t bls_id, uint8_t num_blinks, uint16_t brightness,
                uint16_t on_ms = 0, uint16_t off_ms = 0, bool hold = false) {
    auto bls = GetBtnSwLED(bls_id);
    if (bls) {
      bls.value().get().LEDBlink(num_blinks, brightness, on_ms, off_ms, hold);
    }
  }

  void LEDBlink(uint8_t num_blinks, uint16_t brightness, uint16_t on_ms = 0,
                uint16_t off_ms = 0, bool hold = false) {
    for (auto& bls : bls_) {
      bls.get().LEDBlink(num_blinks, brightness, on_ms, off_ms, hold);
    }
  }

  void LEDPulse(uint8_t bls_id, uint16_t brightness, uint16_t cycle_ms) {
    auto bls = GetBtnSwLED(bls_id);
    if (bls) {
      bls.value().get().LEDPulse(brightness, cycle_ms);
    }
  }

  void LEDPulse(uint16_t brightness, uint16_t cycle_ms) {
    for (auto& bls : bls_) {
      bls.get().LEDPulse(brightness, cycle_ms);
    }
  }

  void LEDSetAmbientBrightness(uint8_t bls_id, uint16_t brightness) {
    auto bls = GetBtnSwLED(bls_id);
    if (bls) {
      bls.value().get().LEDSetAmbientBrightness(brightness);
    }
  }

  void LEDSetAmbientBrightness(uint16_t brightness) {
    for (auto& bls : bls_) {
      bls.get().LEDSetAmbientBrightness(brightness);
    }
  }

 private:
  bool InternalInit() override {
    for (auto& bls : bls_) {
      bls.get().Init();
      bls.get().SetEventCallback(
          std::bind(&BtnSwLEDInput::Callback, this, std::placeholders::_1));
    }
    return true;
  }

  bool InternalStart() override {
    for (auto& bls : bls_) {
      bls.get().Start();
    }
    return true;
  }

  bool InternalStop() override {
    for (auto& bls : bls_) {
      bls.get().Stop();
    }
    SetStopped();
    return true;
  }

  void InternalLoop() override {
    for (auto& bls : bls_) {
      bls.get().Loop();
    }
  }

  void InternalRestart() override {
    for (auto& bls : bls_) {
      bls.get().Restart();
    }
  }

  void Callback(Event event) {
    if (event_callback_) {
      event_callback_(event);
    }
    if (event_callback_secondary_) {
      event_callback_secondary_(event);
    }
  }

  std::array<std::reference_wrapper<BtnSwLED>, N> bls_;
};

#endif  // HOMEBUTTONS_BLS_H

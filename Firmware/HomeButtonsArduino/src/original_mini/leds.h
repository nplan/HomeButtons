#ifndef HOMEBUTTONS_LEDS_H
#define HOMEBUTTONS_LEDS_H

#include <optional>
#include "Arduino.h"
#include "config.h"
#include "component_base.h"
#include "hardware.h"
#include "state_machine.h"

class LED;

namespace LEDSMStates {
class IdleState : public State<LED> {
 public:
  using State<LED>::State;

  void loop() override;

  const char* get_name() override { return "IdleState"; }
};

class OnState : public State<LED> {
 public:
  using State<LED>::State;

  void entry() override;
  void loop() override;

  const char* get_name() override { return "OnState"; }
};

class OffState : public State<LED> {
 public:
  using State<LED>::State;

  void entry() override;
  void loop() override;

  const char* get_name() override { return "OffState"; }
};
}  // namespace LEDSMStates

using LEDStateMachine =
    StateMachine<LED, LEDSMStates::IdleState, LEDSMStates::OnState,
                 LEDSMStates::OffState>;

class LED : public ComponentBase, public LEDStateMachine {
 public:
  LED(const char* name, uint16_t id, HardwareDefinition& hw)
      : ComponentBase(name),
        LEDStateMachine("LED_SM", *this),
        id_(id),
        hw_(hw) {}

  bool InternalInit() override { return true; }
  bool InternalStart() override { return true; }
  bool InternalStop() override;
  void InternalLoop() override;
  void InternalRestart() override {};

  void Blink(uint8_t num_blinks, uint8_t brightness, uint16_t on_ms = 0,
             uint16_t off_ms = 0, bool hold = false);

  uint8_t id() const { return id_; }

 private:
  uint16_t id_ = 0;
  HardwareDefinition& hw_;
  struct LEDBlink {
    uint8_t num_blinks = 0;
    uint8_t brightness = 0;
    uint16_t on_ms = 0;
    uint16_t off_ms = 0;
    bool hold = false;
  };

  enum class SMState { kIdle, kOn, kOff };
  SMState sm_state_ = SMState::kIdle;
  uint32_t last_change_time_ = 0;
  uint8_t current_blink_num_ = 0;

  std::optional<LEDBlink> cmd_blink_;
  std::optional<LEDBlink> current_blink_;

  friend class LEDSMStates::IdleState;
  friend class LEDSMStates::OnState;
  friend class LEDSMStates::OffState;
};

template <uint8_t N_LEDS>
class LEDs : public ComponentBase {
 public:
  LEDs(const char* name, std::array<std::reference_wrapper<LED>, N_LEDS> leds)
      : ComponentBase(name), leds_(leds) {}

  void Blink(uint8_t led_id, uint8_t num_blinks, uint8_t brightness,
             uint16_t on_ms = 0, uint16_t off_ms = 0, bool hold = false) {
    for (auto& led : leds_) {
      if (led.get().id() == led_id) {
        led.get().Blink(num_blinks, brightness, on_ms, off_ms, hold);
        return;
      }
    }
  }

 private:
  bool InternalInit() override {
    for (auto& led : leds_) {
      led.get().Init();
    }
    return true;
  }
  bool InternalStart() override {
    for (auto& led : leds_) {
      led.get().Start();
    }
    return true;
  }
  bool InternalStop() override {
    for (auto& led : leds_) {
      led.get().Stop();
    }
    return true;
  }
  void InternalLoop() override {
    for (auto& led : leds_) {
      led.get().Loop();
    }
    if (cstate() == ComponentState::kCmdStop) {
      bool all_stopped = true;
      for (auto& led : leds_) {
        debug("led %d state: %s", led.get().id(),
              State2Str(led.get().cstate()));
        if (led.get().cstate() != ComponentState::kStopped) {
          all_stopped = false;
          break;
        }
      }
      if (all_stopped) {
        SetStopped();
      }
    }
  }

  void InternalRestart() override {
    for (auto& led : leds_) {
      led.get().Restart();
    }
  }

  std::array<std::reference_wrapper<LED>, N_LEDS> leds_;
};

#endif  // HOMEBUTTONS_LEDS_H

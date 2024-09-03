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

  void entry() override;
  void loop() override;

  const char* get_name() override { return "IdleState"; }
};

class BlinkOnState : public State<LED> {
 public:
  using State<LED>::State;

  void entry() override;
  void loop() override;

  const char* get_name() override { return "BlinkOnState"; }
};

class BlinkOffState : public State<LED> {
 public:
  using State<LED>::State;

  void entry() override;
  void loop() override;

  const char* get_name() override { return "BlinkOffState"; }
};

class ConstOnState : public State<LED> {
 public:
  using State<LED>::State;

  void entry() override;
  void loop() override;

  const char* get_name() override { return "ConstOnState"; }
};

class PulseState : public State<LED> {
 public:
  using State<LED>::State;

  void entry() override;
  void loop() override;

  const char* get_name() override { return "PulseState"; }
};

class TransitionState : public State<LED> {
 public:
  using State<LED>::State;

  void entry() override;
  void loop() override;

  const char* get_name() override { return "TransitionState"; }

 private:
  uint32_t start_time_ = 0;
};

}  // namespace LEDSMStates

enum class LEDBlinkType { kOff, kConstant, kBlink, kPulse };

struct LEDBlink {
  LEDBlinkType type = LEDBlinkType::kOff;
  uint16_t brightness = 0;
  uint8_t num_blinks = 0;
  uint16_t on_ms = 0;
  uint16_t off_ms = 0;
  bool hold = false;
  uint16_t cycle_ms = 0;
};

using LEDStateMachine =
    StateMachine<LED, LEDSMStates::IdleState, LEDSMStates::BlinkOnState,
                 LEDSMStates::BlinkOffState, LEDSMStates::ConstOnState,
                 LEDSMStates::PulseState, LEDSMStates::TransitionState>;

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
  void InternalRestart() override;

  void On(uint16_t brightness);
  void Off();

  void Blink(uint8_t num_blinks, uint16_t brightness, uint16_t on_ms = 0,
             uint16_t off_ms = 0, bool hold = false);

  void Pulse(uint16_t brightness, uint16_t cycle_ms = 1000);

  void SetAmbientBrightness(uint16_t brightness) {
    ambient_brightness_ = brightness;
    debug("LED %d ambient brightness set to: %d", id_, ambient_brightness_);
    if (is_current_state<LEDSMStates::IdleState>()) {
      transition_to<LEDSMStates::IdleState>();
    }
  }

  uint8_t id() const { return id_; }

 private:
  uint16_t id_ = 0;
  HardwareDefinition& hw_;

  uint32_t last_change_time_ = 0;
  uint8_t current_blink_num_ = 0;

  std::optional<LEDBlink> cmd_blink_;
  std::optional<LEDBlink> current_blink_;

  uint8_t ambient_brightness_ = 0;

  friend class LEDSMStates::IdleState;
  friend class LEDSMStates::BlinkOnState;
  friend class LEDSMStates::BlinkOffState;
  friend class LEDSMStates::ConstOnState;
  friend class LEDSMStates::PulseState;
  friend class LEDSMStates::TransitionState;
};

template <uint8_t N_LEDS>
class LEDs : public ComponentBase {
 public:
  LEDs(const char* name, std::array<std::reference_wrapper<LED>, N_LEDS> leds)
      : ComponentBase(name), leds_(leds) {}

  void Blink(uint8_t led_id, uint8_t num_blinks, uint16_t brightness,
             uint16_t on_ms = 0, uint16_t off_ms = 0, bool hold = false) {
    for (auto& led : leds_) {
      if (led.get().id() == led_id) {
        led.get().Blink(num_blinks, brightness, on_ms, off_ms, hold);
        return;
      }
    }
  }

  void On(uint8_t led_id, uint16_t brightness) {
    for (auto& led : leds_) {
      if (led.get().id() == led_id) {
        led.get().On(brightness);
        return;
      }
    }
  }

  void Off(uint8_t led_id) {
    for (auto& led : leds_) {
      if (led.get().id() == led_id) {
        led.get().Off();
        return;
      }
    }
  }

  void Pulse(uint8_t led_id, uint16_t brightness, uint16_t cycle_ms = 1000) {
    for (auto& led : leds_) {
      if (led.get().id() == led_id) {
        led.get().Pulse(brightness, cycle_ms);
        return;
      }
    }
  }

  void SetAmbientBrightness(uint8_t led_id, uint16_t brightness) {
    for (auto& led : leds_) {
      if (led.get().id() == led_id) {
        led.get().SetAmbientBrightness(brightness);
        return;
      }
    }
  }

  void AllOn(uint16_t brightness) {
    for (auto& led : leds_) {
      led.get().On(brightness);
    }
  }

  void AllOff() {
    for (auto& led : leds_) {
      led.get().Off();
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

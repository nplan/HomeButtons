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

 private:
  uint8_t brightness = 0;
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

  void On(uint8_t brightness = 0);
  void Off();

  void Blink(uint8_t num_blinks, uint8_t brightness = 0, uint16_t on_ms = 0,
             uint16_t off_ms = 0, bool hold = false);

  void Pulse(uint8_t brightness = 0, uint16_t cycle_ms = 1000);

  void SetDefaultBrightness(uint8_t brightness);
  void SetAmbientBrightness(uint8_t brightness);

  uint8_t id() const { return id_; }

 private:
  uint16_t id_ = 0;
  HardwareDefinition& hw_;

  uint32_t last_change_time_ = 0;
  uint8_t current_blink_num_ = 0;

  std::optional<LEDBlink> cmd_blink_;
  std::optional<LEDBlink> current_blink_;

  uint8_t default_brightness_ = 0;
  uint8_t ambient_brightness_ = 0;

  friend class LEDSMStates::IdleState;
  friend class LEDSMStates::BlinkOnState;
  friend class LEDSMStates::BlinkOffState;
  friend class LEDSMStates::ConstOnState;
  friend class LEDSMStates::PulseState;
  friend class LEDSMStates::TransitionState;
};
#endif  // HOMEBUTTONS_LEDS_H

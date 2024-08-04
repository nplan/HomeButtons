#ifndef HOMEBUTTONS_BUTTONS_H
#define HOMEBUTTONS_BUTTONS_H

#include "Arduino.h"
#include "logger.h"
#include "types.h"
#include "user_input.h"
#include "hardware.h"

static constexpr uint32_t kDebounceTimeout = 50L;
static constexpr uint32_t kPressTimeout = 500L;
static constexpr uint32_t kTriggerInterval = 250L;

class Button : public UserInput {
 public:
  Button(const char* name, uint16_t id, HardwareDefinition& hw)
      : UserInput(name), id_(id), hw_(hw) {}
  void InitPress();

  uint16_t id() const { return id_; }
  uint8_t pin() { return hw_.button_pin(id_); }

 private:
  bool InternalInit() override { return true; }
  bool InternalStart() override;
  bool InternalStop() override;
  void InternalLoop() override;
  void InternalRestart() override;

  void TriggerClick(uint32_t duration, uint16_t num_clicks,
                    bool finished = true);

  uint16_t id_ = 0;
  HardwareDefinition& hw_;
  bool rising_flag_ = false;
  bool falling_flag_ = false;

  uint32_t sm_time_ = 0;
  uint8_t sm_state_ = 0;
  uint8_t num_clicks_ = 0;
  uint32_t last_trigger_time_ = 0;
  uint32_t press_start_time_ = 0;

  void IRAM_ATTR ISR();
};

template <uint8_t N_BTNS>
class ButtonInput : public UserInput {
 public:
  ButtonInput(const char* name,
              std::array<std::reference_wrapper<Button>, N_BTNS> buttons)
      : UserInput(name), buttons_(buttons) {}
  uint16_t id_from_pin(uint8_t pin) {
    for (auto& btn : buttons_) {
      if (btn.get().pin() == pin) {
        return btn.get().id();
      }
    }
    return 0;
  }
  void InitPress(uint8_t btn_id) {
    for (auto& btn : buttons_) {
      if (btn.get().id() == btn_id) {
        btn.get().InitPress();
        return;
      }
    }
  }

 private:
  bool InternalInit() override {
    for (auto& btn : buttons_) {
      btn.get().Init();
      btn.get().SetEventCallback(
          std::bind(&ButtonInput::BtnCallback, this, std::placeholders::_1));
    }
    return true;
  }
  bool InternalStart() override {
    for (auto& btn : buttons_) {
      btn.get().Start();
    }
    return true;
  }
  bool InternalStop() override {
    for (auto& btn : buttons_) {
      btn.get().Stop();
    }
    SetStopped();
    return true;
  }
  void InternalLoop() override {
    for (auto& btn : buttons_) {
      btn.get().Loop();
    }
  }

  void InternalRestart() override {
    for (auto& btn : buttons_) {
      btn.get().Restart();
    }
  }

  void BtnCallback(Event event) {
    if (event_callback_) {
      event_callback_(event);
    }
    if (event_callback_secondary_) {
      event_callback_secondary_(event);
    }
  }

  std::array<std::reference_wrapper<Button>, N_BTNS> buttons_;
};

#endif  // HOMEBUTTONS_BUTTONS_H

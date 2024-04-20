#ifndef HOMEBUTTONS_BUTTONS_H
#define HOMEBUTTONS_BUTTONS_H

#include "Arduino.h"
#include "logger.h"
#include "types.h"
#include "user_input.h"

static constexpr uint32_t LONG_2S_TIME = 2000L;
static constexpr uint32_t LONG_5S_TIME = 5000L;
static constexpr uint32_t LONG_10S_TIME = 10000L;
static constexpr uint32_t LONG_20S_TIME = 20000L;

static constexpr uint32_t DEBOUNCE_TIMEOUT = 50L;
static constexpr uint32_t PRESS_TIMEOUT = 500L;

class Button : public UserInput {
 public:
  Button(const char* name) : UserInput(name) {}
  bool Init(uint8_t pin, uint16_t id, bool active_high = true);
  void init_press();

  uint8_t pin() const { return pin_; }
  uint16_t id() const { return id_; }

 private:
  bool InternalInit() override { return true; }
  bool InternalStart() override;
  bool InternalStop() override;
  void InternalLoop() override;

  void TriggerClick(uint32_t duration, uint16_t num_clicks,
                    bool finished = true);
  void TriggerEvent(Event event);

  uint8_t pin_ = 0;
  uint16_t id_ = 0;
  bool active_high_ = false;
  bool rising_flag_ = false;
  bool falling_flag_ = false;

  bool click_started_ = false;

  uint8_t sm_state_ = 0;
  uint32_t press_start_time_ = 0;
  uint32_t last_trigger_time_ = 0;

  void IRAM_ATTR ISR();
  bool ReadPin() const;
};

template <uint8_t N_BTNS>
class ButtonHandler : public UserInput {
 public:
  ButtonHandler() : UserInput("BTN HNDL") {}
  // (pin, id, active_high)
  void Init(std::tuple<uint8_t, uint16_t, boolean> config[N_BTNS]) {
    for (uint8_t i = 0; i < N_BTNS; i++) {
      buttons_[i].begin(std::get<0>(config[i]), std::get<1>(config[i]),
                        std::get<2>(config[i]));
    }
    UserInput::Init();
    return true;
  }

  void end() {
    for (auto& button : buttons_) {
      button.end();
    }
    active_button_ = nullptr;
    debug("ended");
  }

  void update() {
    for (auto& button : buttons_) {
      button.update();
    }
    for (auto& button : buttons_) {
      if (button.get_action() != ClickAction::NONE) {
        active_button_ = &button;
        break;
      }
    }
  }

  void clear() {
    for (auto& button : buttons_) {
      button.clear();
    }
    active_button_ = nullptr;
    debug("cleared");
  }

  boolean is_press_finished() {
    return active_button_ && active_button_->is_press_finished();
  }

  boolean is_press_in_progress() {
    return get_event().action != ClickAction::NONE;
  }

  ClickEvent get_event() {
    if (!active_button_) {
      return ClickEvent{0, ClickAction::NONE};
    }
    return ClickEvent{active_button_->get_id(), active_button_->get_action()};
  }

  void init_press(uint16_t pin) {
    for (auto& button : buttons_) {
      if (button.get_pin() == pin) {
        button.init_press();
        break;
      }
    }
  }

  boolean is_button(uint16_t pin) {
    for (auto& button : buttons_) {
      if (button.get_pin() == pin) {
        return true;
      }
    }
    return false;
  }

 private:
  bool InternalInit() override { return true; }
  bool InternalStart() override { return true; }

  std::array<Button, N_BTNS> buttons_;
  Button* active_button_ = nullptr;
};

#endif  // HOMEBUTTONS_BUTTONS_H

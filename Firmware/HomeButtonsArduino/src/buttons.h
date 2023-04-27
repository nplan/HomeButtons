#ifndef HOMEBUTTONS_BUTTONS_H
#define HOMEBUTTONS_BUTTONS_H

#include "Arduino.h"
#include "logger.h"

static constexpr uint32_t LONG_1_TIME = 2000L;
static constexpr uint32_t LONG_2_TIME = 5000L;
static constexpr uint32_t LONG_3_TIME = 10000L;
static constexpr uint32_t LONG_4_TIME = 20000L;

static constexpr uint32_t DEBOUNCE_TIMEOUT = 50L;
static constexpr uint32_t PRESS_TIMEOUT = 500L;

class Button : public Logger {
 public:
  enum ButtonAction {
    IDLE,
    SINGLE,
    DOUBLE,
    TRIPLE,
    QUAD,
    LONG_1,
    LONG_2,
    LONG_3,
    LONG_4
  };
  Button() : Logger("BTN") {}
  void begin(uint8_t pin, uint16_t id, bool active_high = true);
  void init_press();
  void end();
  void update();
  ButtonAction get_action() const;
  bool is_press_finished() const;
  void clear() { _reset(); }
  uint8_t get_pin() const;
  uint16_t get_id() const;
  static const char* get_action_name(ButtonAction action);
  static uint8_t get_action_multi_count(ButtonAction action);

 private:
  bool begun = false;

  uint8_t pin = 0;
  uint16_t id = 0;
  bool active_high = false;

  uint8_t state_machine_state = 0;

  uint32_t press_start_time = 0;
  uint32_t release_start_time = 0;

  ButtonAction action = IDLE;
  bool press_finished = false;

  bool rising_flag = false;
  bool falling_flag = false;

  void IRAM_ATTR _isr();
  bool _read_pin() const;
  void _reset();
};

struct ButtonEvent {
  uint16_t id;
  Button::ButtonAction action;
};

template <uint8_t N_BTNS>
class ButtonHandler : public Logger {
 public:
  ButtonHandler() : Logger("BTN_H") {}
  void begin(std::tuple<uint8_t, uint16_t, boolean> config[N_BTNS]) {
    for (uint8_t i = 0; i < N_BTNS; i++) {
      buttons_[i].begin(std::get<0>(config[i]), std::get<1>(config[i]),
                        std::get<2>(config[i]));
    }
    debug("begun");
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
      if (button.get_action() != Button::IDLE) {
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

  boolean is_press_in_in_progress() {
    return get_event().action != Button::IDLE;
  }

  ButtonEvent get_event() {
    if (!active_button_) {
      return ButtonEvent{0, Button::IDLE};
    }
    return ButtonEvent{active_button_->get_id(), active_button_->get_action()};
  }

  void init_press(uint16_t pin) {
    for (auto& button : buttons_) {
      if (button.get_pin() == pin) {
        button.init_press();
        break;
      }
    }
  }

 private:
  std::array<Button, N_BTNS> buttons_;
  Button* active_button_ = nullptr;
};

#endif  // HOMEBUTTONS_BUTTONS_H

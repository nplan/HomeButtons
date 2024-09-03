#ifndef HOMEBUTTONS_STATEMACHINE_H
#define HOMEBUTTONS_STATEMACHINE_H

#include <tuple>
#include <variant>
#include <stdio.h>

#include "logger.h"

template <typename Base>
class State;

template <typename Base, typename... States>
class StateMachine;

template <typename Base>
class State {
 public:
  using SM = Base;
  explicit State(Base &base) : base_(base) {}

  virtual void entry() {}
  virtual void loop() {}
  virtual void exit() {}

  virtual const char *get_name() = 0;

  template <typename NextState>
  void transition_to() {
    base_.template transition_to<NextState>();
  }

 protected:
  Base &base_;
  Base &sm() { return base_; }
};

template <typename Base, typename... States>
class StateMachine {
 public:
  StateMachine(const char *name, Base &base)
      : current_state_(&std::get<0>(states_)),
        states_(States(base)...),
        base_(base) {
    static_assert(std::is_base_of<Logger, Base>::value);
    snprintf(name_, MAX_NAME_LENGTH, "%s", name);
  }

  template <typename NextState>
  void transition_to() {
    _exit_state(current_state_);

    current_state_ = &std::get<NextState>(states_);

    base_.debug("Entering state %s::%s", name_,
                std::visit([](auto statePtr) { return statePtr->get_name(); },
                           current_state_));
    std::visit([](auto statePtr) { statePtr->entry(); }, current_state_);
  }

  void loop() {
    if (first_run_) {
      first_run_ = false;
      _enter_state(current_state_);
    }
    std::visit([](auto statePtr) { statePtr->loop(); }, current_state_);
  }

  template <typename State>
  bool is_current_state() const {
    return std::holds_alternative<State *>(current_state_);
  }

  void _enter_state(std::variant<States *...> state) {
    base_.debug(
        "Entering state %s::%s", name_,
        std::visit([](auto statePtr) { return statePtr->get_name(); }, state));
    std::visit([](auto statePtr) { statePtr->entry(); }, state);
  }

  void _exit_state(std::variant<States *...> state) {
    base_.debug(
        "Leaving state %s::%s", name_,
        std::visit([](auto statePtr) { return statePtr->get_name(); }, state));
    std::visit([](auto statePtr) { statePtr->exit(); }, state);
  }

 protected:
  std::variant<States *...> current_state_;
  std::tuple<States...> states_;
  static constexpr size_t MAX_NAME_LENGTH = 32;
  char name_[MAX_NAME_LENGTH];
  Base &base_;
  bool first_run_ = true;
};

#endif  // HOMEBUTTONS_STATEMACHINE_H

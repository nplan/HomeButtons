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
    base_.info("Leaving state %s::%s", name_,
               std::visit([](auto statePtr) { return statePtr->get_name(); },
                          current_state_));
    std::visit([](auto statePtr) { statePtr->exit(); }, current_state_);

    current_state_ = &std::get<NextState>(states_);

    base_.info("Entering state %s::%s", name_,
               std::visit([](auto statePtr) { return statePtr->get_name(); },
                          current_state_));
    std::visit([](auto statePtr) { statePtr->entry(); }, current_state_);
  }

  void loop() {
    std::visit([](auto statePtr) { statePtr->loop(); }, current_state_);
  }

  template <typename State>
  bool is_current_state() const {
    return std::holds_alternative<State *>(current_state_);
  }

 protected:
  std::variant<States *...> current_state_;
  std::tuple<States...> states_;
  static constexpr size_t MAX_NAME_LENGTH = 32;
  char name_[MAX_NAME_LENGTH];
  Base &base_;
};

#endif  // HOMEBUTTONS_STATEMACHINE_H

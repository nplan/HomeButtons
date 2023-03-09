#ifndef HOMEBUTTONS_STATEMACHINE_H
#define HOMEBUTTONS_STATEMACHINE_H

#include <tuple>
#include <variant>
#include <stdio.h>

template <typename StateMachineDefinition>
class State;

template <typename... States>
class StateMachine;

template <typename StateMachineDefinition>
class State {
 public:
  using SM = StateMachineDefinition;
  explicit State(StateMachineDefinition &stateMachineDefinition)
      : state_machine_(stateMachineDefinition) {}

  virtual void entry() {}
  virtual void executeOnce() {}
  virtual void exit() {}

  virtual const char *getName() = 0;

  template <typename NextState>
  void transitionTo() {
    state_machine_.template transitionTo<NextState>();
  }

 protected:
  StateMachineDefinition &state_machine_;
  StateMachineDefinition &sm() { return state_machine_; }
};

template <typename... States>
class StateMachine {
 public:
  template <typename StateMachineType>
  StateMachine(const char *name, StateMachineType &stateMachineType)
      : m_currentState(&std::get<0>(m_states)),
        m_states(States(stateMachineType)...) {
    snprintf(m_name, MAX_NAME_LENGTH, "%s", name);
  }

  template <typename NextState>
  void transitionTo() {
    log_i("Leaving state %s::%s", m_name,
          std::visit([](auto statePtr) { return statePtr->getName(); },
                     m_currentState));
    std::visit([](auto statePtr) { statePtr->exit(); }, m_currentState);

    m_currentState = &std::get<NextState>(m_states);

    log_i("Entering state %s::%s", m_name,
          std::visit([](auto statePtr) { return statePtr->getName(); },
                     m_currentState));
    std::visit([](auto statePtr) { statePtr->entry(); }, m_currentState);
  }

  void executeOnce() {
    std::visit([](auto statePtr) { statePtr->executeOnce(); }, m_currentState);
  }

  template <typename State>
  bool isCurrentState() const {
    return std::holds_alternative<State *>(m_currentState);
  }

 protected:
  std::variant<States *...> m_currentState;
  std::tuple<States...> m_states;
  static constexpr size_t MAX_NAME_LENGTH = 32;
  char m_name[MAX_NAME_LENGTH];
};

#endif  // HOMEBUTTONS_STATEMACHINE_H

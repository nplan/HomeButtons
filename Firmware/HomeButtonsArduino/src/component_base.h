#ifndef COMPONENT_BASE_H
#define COMPONENT_BASE_H

#include "logger.h"

class ComponentBase : public Logger {
 public:
  enum class State { kUninitialized, kInitialized, kStopped, kRunning };
  static const char* State2Str(State state) {
    switch (state) {
      case State::kUninitialized:
        return "Uninitialized";
      case State::kInitialized:
        return "Initialized";
      case State::kStopped:
        return "Stopped";
      case State::kRunning:
        return "Running";
      default:
        return "Unknown";
    }
  }
  ComponentBase(const char* name) : Logger(name) {}
  bool Init() {
    if (state_ == State::kUninitialized) {
      if (InternalInit()) {
        state_ = State::kInitialized;
        debug("initialized");
        return true;
      } else {
        debug("failed to initialize");
        return false;
      }
    } else {
      debug("already initialized");
      return true;
    }
  }
  bool Start() {
    if (state_ == State::kStopped || state_ == State::kInitialized) {
      if (InternalStart()) {
        state_ = State::kRunning;
        debug("started");
        return true;
      } else {
        debug("failed to start");
        return false;
      }
    } else if (state_ == State::kRunning) {
      debug("already started");
      return true;
    } else {
      debug("can't start, state: %s", State2Str(state_));
      return false;
    }
  }
  bool Stop() {
    if (state_ == State::kRunning) {
      if (InternalStop()) {
        state_ = State::kStopped;
        debug("stopped");
        return true;
      } else {
        debug("failed to stop");
        return false;
      }
    } else if (state_ == State::kStopped) {
      debug("already stopped");
      return true;
    } else {
      debug("can't stop, state: %s", State2Str(state_));
      return false;
    }
  }
  void Loop() {
    if (state_ == State::kRunning) {
      InternalLoop();
    }
  }
  State state() const { return state_; }
  virtual ~ComponentBase() = default;

 private:
  virtual bool InternalInit() = 0;
  virtual bool InternalStart() = 0;
  virtual bool InternalStop() = 0;
  virtual void InternalLoop() = 0;
  State state_ = State::kInitialized;
};

#endif

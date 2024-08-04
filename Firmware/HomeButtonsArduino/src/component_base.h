#ifndef COMPONENT_BASE_H
#define COMPONENT_BASE_H

#include "logger.h"

class ComponentBase : public Logger {
 public:
  enum class ComponentState {
    kUninitialized,
    kInitialized,
    kCmdStop,
    kStopped,
    kRunning
  };
  static const char* State2Str(ComponentState state) {
    switch (state) {
      case ComponentState::kUninitialized:
        return "Uninitialized";
      case ComponentState::kInitialized:
        return "Initialized";
      case ComponentState::kCmdStop:
        return "CmdStop";
      case ComponentState::kStopped:
        return "Stopped";
      case ComponentState::kRunning:
        return "Running";
      default:
        return "Unknown";
    }
  }
  ComponentBase(const char* name) : Logger(name) {}

  bool Init() {
    if (cstate_ == ComponentState::kUninitialized) {
      if (InternalInit()) {
        cstate_ = ComponentState::kInitialized;
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
    if (cstate_ == ComponentState::kStopped ||
        cstate_ == ComponentState::kInitialized) {
      if (InternalStart()) {
        cstate_ = ComponentState::kRunning;
        debug("started");
        return true;
      } else {
        debug("failed to start");
        return false;
      }
    } else if (cstate_ == ComponentState::kRunning) {
      debug("already started");
      return true;
    } else {
      debug("can't start, state: %s", State2Str(cstate_));
      return false;
    }
  }

  bool Stop() {
    if (cstate_ == ComponentState::kRunning) {
      if (InternalStop()) {
        if (cstate_ == ComponentState::kStopped) {
          // stopped immediately in InternalStop()
          debug("stopped immediately");
        } else {
          cstate_ = ComponentState::kCmdStop;
          debug("cmd stop accepted");
        }
        return true;
      } else {
        debug("cmd stop denied");
        return false;
      }
    } else if (cstate_ == ComponentState::kStopped) {
      debug("already stopped");
      return true;
    } else {
      debug("can't stop, state: %s", State2Str(cstate_));
      return false;
    }
  }

  void Loop() {
    if (cstate_ == ComponentState::kRunning ||
        cstate_ == ComponentState::kCmdStop) {
      InternalLoop();
    }
  }

  void Restart() {
    InternalRestart();
    debug("restarted");
  }

  ComponentState cstate() const { return cstate_; }
  virtual ~ComponentBase() = default;
  ComponentBase() = delete;

 protected:
  void SetStopped() {
    cstate_ = ComponentState::kStopped;
    debug("stopped");
  }

 private:
  virtual bool InternalInit() = 0;
  virtual bool InternalStart() = 0;
  // should instruct loop to stop or stop immediately
  virtual bool InternalStop() = 0;
  virtual void InternalLoop() = 0;
  virtual void InternalRestart() = 0;

  ComponentState cstate_ = ComponentState::kUninitialized;
};

#endif

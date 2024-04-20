#ifndef USER_INPUT_H
#define USER_INPUT_H

#include "component_base.h"

class UserInput : public ComponentBase {
 public:
  enum class EventType {
    kNone,
    kTouchStart,
    kTouchEnd,
    kTap,
    kSwipeUp,
    kSwipeDown,
    kSwipeLeft,
    kSwipeRight,
    kClickSingle,
    kClickDouble,
    kClickTriple,
    kClickQuad,
    kHoldLong2s,
    kClickLong2s,
    kHoldLong5s,
    kClickLong5s,
    kHoldLong10s,
    kClickLong10s,
    kHoldLong20s,
    kClickLong20s
  };

  static const char* EventType2Str(EventType type) {
    switch (type) {
      case EventType::kNone:
        return "None";
      case EventType::kTouchStart:
        return "TouchStart";
      case EventType::kTouchEnd:
        return "TouchEnd";
      case EventType::kTap:
        return "Tap";
      case EventType::kSwipeUp:
        return "SwipeUp";
      case EventType::kSwipeDown:
        return "SwipeDown";
      case EventType::kSwipeLeft:
        return "SwipeLeft";
      case EventType::kSwipeRight:
        return "SwipeRight";
      case EventType::kClickSingle:
        return "ClickSingle";
      case EventType::kClickDouble:
        return "ClickDouble";
      case EventType::kClickTriple:
        return "ClickTriple";
      case EventType::kClickQuad:
        return "ClickQuad";
      case EventType::kHoldLong2s:
        return "HoldLong2s";
      case EventType::kClickLong2s:
        return "ClickLong2s";
      case EventType::kHoldLong5s:
        return "HoldLong5s";
      case EventType::kClickLong5s:
        return "ClickLong5s";
      case EventType::kHoldLong10s:
        return "HoldLong10s";
      case EventType::kClickLong10s:
        return "ClickLong10s";
      case EventType::kHoldLong20s:
        return "HoldLong20s";
      case EventType::kClickLong20s:
        return "ClickLong20s";
      default:
        return "Unknown";
    }
  }

  struct TouchPoint {
    uint16_t x = 0;
    uint16_t y = 0;
  };

  struct Event {
    EventType type = EventType::kNone;
    TouchPoint point = {0, 0};
    uint16_t btn_num = 0;
  };

  UserInput(const char* name) : ComponentBase(name) {}
  void SetEventCallback(std::function<void(Event)> callback) {
    event_callback_ = callback;
  }
  void SetEventCallbackSecondary(std::function<void(Event)> callback) {
    event_callback_secondary_ = callback;
  }

 private:
  std::function<void(Event)> event_callback_;
  std::function<void(Event)> event_callback_secondary_;
};

#endif

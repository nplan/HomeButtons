#ifndef USER_INPUT_H
#define USER_INPUT_H

#include "component_base.h"

static constexpr uint32_t kLong2sTime = 2000L;
static constexpr uint32_t kLong5sTime = 5000L;
static constexpr uint32_t kLong10sTime = 10000L;
static constexpr uint32_t kLong20sTime = 20000L;

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
    kHoldLong5s,
    kHoldLong10s,
    kHoldLong20s,
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
      case EventType::kHoldLong5s:
        return "HoldLong5s";
      case EventType::kHoldLong10s:
        return "HoldLong10s";
      case EventType::kHoldLong20s:
        return "HoldLong20s";
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
    uint16_t btn_id = 0;
    bool final = false;
  };

  UserInput(const char* name) : ComponentBase(name) {}
  void SetEventCallback(std::function<void(Event)> callback) {
    event_callback_ = callback;
  }
  void SetEventCallbackSecondary(std::function<void(Event)> callback) {
    event_callback_secondary_ = callback;
  }

  static uint8_t EventType2NumClicks(EventType type) {
    switch (type) {
      case EventType::kClickSingle:
        return 1;
      case EventType::kClickDouble:
        return 2;
      case EventType::kClickTriple:
        return 3;
      case EventType::kClickQuad:
        return 4;
      default:
        return 0;
    }
  }

 protected:
  std::function<void(Event)> event_callback_;
  std::function<void(Event)> event_callback_secondary_;

  void TriggerEvent(Event event) {
    info("UI event: %s, point (%d, %d), btn_id: %d, final: %d",
         EventType2Str(event.type), event.point.x, event.point.y, event.btn_id,
         event.final);
    if (event_callback_) {
      event_callback_(event);
    }
    if (event_callback_secondary_) {
      event_callback_secondary_(event);
    }
  }
};

#endif

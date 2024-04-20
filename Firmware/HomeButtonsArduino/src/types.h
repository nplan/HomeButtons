#ifndef HOMEBUTTONS_TYPES_H
#define HOMEBUTTONS_TYPES_H

#include "static_string.h"
#include "config.h"

using SerialNumber = StaticString<8>;
using RandomID = StaticString<6>;
using ModelName = StaticString<20>;
using ModelID = StaticString<2>;
using HWVersion = StaticString<3>;
using UniqueID = StaticString<21>;

using DeviceName = StaticString<20>;
using ButtonLabel = StaticString<BTN_LABEL_MAXLEN>;
using MDIName = StaticString<48>;
using UserMessage = StaticString<USER_MSG_MAXLEN>;

using TouchActionString = StaticString<16>;
using ClickActionString = StaticString<16>;

/*
enum class ClickAction {
  NONE,
  SINGLE,
  DOUBLE,
  TRIPLE,
  QUAD,
  CLICK_LONG_2S,
  CLICK_LONG_5S,
  CLICK_LONG_10S,
  CLICK_LONG_20S
};

inline ClickActionString click_action_to_string(ClickAction action) {
  switch (action) {
    case ClickAction::NONE:
      return ClickActionString{"NONE"};
    case ClickAction::SINGLE:
      return ClickActionString{"SINGLE"};
    case ClickAction::DOUBLE:
      return ClickActionString{"DOUBLE"};
    case ClickAction::TRIPLE:
      return ClickActionString{"TRIPLE"};
    case ClickAction::QUAD:
      return ClickActionString{"QUAD"};
    case ClickAction::CLICK_LONG_2S:
      return ClickActionString{"CLICK_LONG_2S"};
    case ClickAction::CLICK_LONG_5S:
      return ClickActionString{"CLICK_LONG_5S"};
    case ClickAction::CLICK_LONG_10S:
      return ClickActionString{"CLICK_LONG_10S"};
    case ClickAction::CLICK_LONG_20S:
      return ClickActionString{"CLICK_LONG_20S"};
    default:
      return ClickActionString{"UNKNOWN"};
  }
}

struct ClickEvent {
  uint16_t btn_num = 0;
  ClickAction action = ClickAction::NONE;
};

enum class UserInputEventType {
  NONE,
  TOUCH_START,
  TOUCH_END,
  TAP,
  SWIPE_UP,
  SWIPE_DOWN,
  SWIPE_LEFT,
  SWIPE_RIGHT,
  CLICK_SINGLE,
  CLICK_DOUBLE,
  CLICK_TRIPLE,
  CLICK_QUAD,
  HOLD_LONG_2S,
  CLICK_LONG_2S,
  HOLD_LONG_5S,
  CLICK_LONG_5S,
  HOLD_LONG_10S,
  CLICK_LONG_10S,
  HOLD_LONG_20S,
  CLICK_LONG_20S
};

inline TouchActionString user_input_event_type_to_string(
    UserInputEventType type) {
  switch (type) {
    case UserInputEventType::NONE:
      return TouchActionString{"NONE"};
    case UserInputEventType::TOUCH_START:
      return TouchActionString{"TOUCH_START"};
    case UserInputEventType::TOUCH_END:
      return TouchActionString{"TOUCH_END"};
    case UserInputEventType::TAP:
      return TouchActionString{"TAP"};
    case UserInputEventType::SWIPE_UP:
      return TouchActionString{"SWIPE_UP"};
    case UserInputEventType::SWIPE_DOWN:
      return TouchActionString{"SWIPE_DOWN"};
    case UserInputEventType::SWIPE_LEFT:
      return TouchActionString{"SWIPE_LEFT"};
    case UserInputEventType::SWIPE_RIGHT:
      return TouchActionString{"SWIPE_RIGHT"};
    case UserInputEventType::CLICK_SINGLE:
      return TouchActionString{"CLICK_SINGLE"};
    case UserInputEventType::CLICK_DOUBLE:
      return TouchActionString{"CLICK_DOUBLE"};
    case UserInputEventType::CLICK_TRIPLE:
      return TouchActionString{"CLICK_TRIPLE"};
    case UserInputEventType::CLICK_QUAD:
      return TouchActionString{"CLICK_QUAD"};
    case UserInputEventType::HOLD_LONG_2S:
      return TouchActionString{"HOLD_LONG_2S"};
    case UserInputEventType::CLICK_LONG_2S:
      return TouchActionString{"CLICK_LONG_2S"};
    case UserInputEventType::HOLD_LONG_5S:
      return TouchActionString{"HOLD_LONG_5S"};
    case UserInputEventType::CLICK_LONG_5S:
      return TouchActionString{"CLICK_LONG_5S"};
    case UserInputEventType::HOLD_LONG_10S:
      return TouchActionString{"HOLD_LONG_10S"};
    case UserInputEventType::CLICK_LONG_10S:
      return TouchActionString{"CLICK_LONG_10S"};
    case UserInputEventType::HOLD_LONG_20S:
      return TouchActionString{"HOLD_LONG_20S"};
    case UserInputEventType::CLICK_LONG_20S:
      return TouchActionString{"CLICK_LONG_20S"};
    default:
      return TouchActionString{"UNKNOWN"};
  }
}

struct TouchPoint {
  uint16_t x = 0;
  uint16_t y = 0;
};

struct UserInputEvent {
  UserInputEventType type = UserInputEventType::NONE;
  TouchPoint touch_point{};
};
*/

enum class DisplayPage {
  EMPTY,
  MAIN,
  INFO,
  DEVICE_INFO,
  MESSAGE,
  MESSAGE_LARGE,
  ERROR,
  WELCOME,
  SETTINGS,
  AP_CONFIG,
  WEB_CONFIG,
  TEST
};

struct UIState {
  static constexpr size_t MAX_MESSAGE_SIZE = 64;
  using MessageType = StaticString<MAX_MESSAGE_SIZE>;

  DisplayPage page = DisplayPage::EMPTY;
  MessageType message{};
  MDIName mdi_name{};
  uint16_t mdi_size = 0;
  bool disappearing = false;
  uint32_t appear_time = 0;
  uint32_t disappear_timeout = 0;
};

enum class LabelType : uint8_t { None, Text, Icon, Mixed };

#endif  // HOMEBUTTONS_TYPES_H;
#ifndef HOMEBUTTONS_TOUCH_H
#define HOMEBUTTONS_TOUCH_H

#include <Wire.h>
#include <FT6X36.h>
#include <functional>

#include "user_input.h"
#include "hardware.h"
#include "freertos/semphr.h"

static constexpr uint32_t kDebounceTimeout = 50L;
static constexpr uint32_t kPressTimeout = 500L;
static constexpr uint32_t kTriggerInterval = 250L;

static constexpr uint32_t kSwipeTimeout = 500L;
static constexpr uint16_t kSwipeDistance = 50L;

static constexpr uint32_t kLong2sTime = 2000L;
static constexpr uint32_t kLong5sTime = 5000L;
static constexpr uint32_t kLong10sTime = 10000L;
static constexpr uint32_t kLong20sTime = 20000L;

class TouchInput : public UserInput {
 public:
  TouchInput(HardwareDefinition hw) : UserInput("TOUCH") { hw_ = hw; }
  bool Init(uint8_t click_pin, uint8_t int_pin, bool active_high = true);

  void touch_handler(TPoint point, TEvent e);

 private:
  bool InternalInit() override { return true; };
  bool InternalStart() override;
  bool InternalStop() override;
  void InternalLoop() override;

  FT6X36* touch_controller_;
  HardwareDefinition hw_;

  uint8_t click_pin_;
  bool active_high_;
  bool rising_flag_ = false;
  bool falling_flag_ = false;

  TouchPoint last_touch_point_ = {0, 0};
  TouchPoint touch_start_point_ = {0, 0};
  uint32_t touch_start_time_ = 0;
  bool touch_started_ = false;

  bool click_started_ = false;

  void IRAM_ATTR _click_isr();
  bool _read_pin() const;
  void _btn_update();
  void _trigger_click(uint32_t duration, uint16_t num_clicks,
                      bool finished = true);
  void _trigger_touch(TEvent event, TouchPoint point);
  void _trigger_event(Event event);
  static uint8_t Touch2BtnNum(TouchPoint point);
};

#endif
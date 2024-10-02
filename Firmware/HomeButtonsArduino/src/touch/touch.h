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

class TouchInput : public UserInput {
 public:
  TouchInput(HardwareDefinition& hw) : UserInput("TOUCH") { hw_ = hw; }
  void TouchHandler(TPoint point, TEvent e);

 private:
  bool InternalInit() override { return true; };
  bool InternalStart() override;
  bool InternalStop() override;
  void InternalLoop() override;

  FT6X36* touch_controller_;
  HardwareDefinition& hw_;

  bool rising_flag_ = false;
  bool falling_flag_ = false;

  TouchPoint last_touch_point_ = {0, 0};
  TouchPoint touch_start_point_ = {0, 0};
  uint32_t touch_start_time_ = 0;
  bool touch_started_ = false;

  bool click_started_ = false;

  uint32_t btn_sm_time_ = 0;
  uint8_t btn_sm_state_ = 0;
  uint8_t btn_num_clicks_ = 0;
  uint32_t btn_last_trigger_time_ = 0;
  uint32_t btn_press_start_time_ = 0;

  void IRAM_ATTR ClickISR();
  bool ReadPin() const;
  void BtnUpdate();
  void TriggerClick(uint32_t duration, uint16_t num_clicks,
                    bool finished = true);
  void TriggerTouch(TEvent event, TouchPoint point);
  void TriggerEvent(Event event);
  static uint8_t Touch2BtnNum(TouchPoint point);
};

#endif
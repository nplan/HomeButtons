#ifndef HOMEBUTTONS_APP_H
#define HOMEBUTTONS_APP_H

#include <array>
#include "state.h"
#include "buttons.h"
#include "leds.h"
#include "network.h"
#include "display.h"
#include "mqtt_helper.h"
#include "logger.h"
#include "hardware.h"

enum class BootCause { RESET, TIMER, BUTTON };

class App : public Logger {
 public:
  App();
  App(const App&) = delete;
  void setup();

 private:
  void _start_esp_sleep();
  void _go_to_sleep();
  std::pair<BootCause, Button*> _determine_boot_cause();
  void _log_stack_status() const;

  void _begin_buttons();
  void _end_buttons();

  static void _button_task(void* app);
  static void _leds_task(void* app);
  static void _display_task(void* app);
  static void _network_task(void* app);

  void _start_button_task();
  void _start_leds_task();
  void _start_display_task();
  void _start_network_task();

  void _main_task();
  static void _main_task_helper(void* app) {
    static_cast<App*>(app)->_main_task();
  }

  void _publish_sensors();
  void _publish_awake_mode_avlb();
  void _mqtt_callback(const char* topic, const char* payload);
  void _net_on_connect();

  enum class StateMachineState {
    AWAIT_NET_CONNECT,
    AWAIT_USER_INPUT_START,
    AWAIT_USER_INPUT_FINISH,
    CMD_SHUTDOWN,
    AWAIT_NET_DISCONNECT,
    AWAIT_SHUTDOWN,
    AWAIT_FACTORY_RESET,
  };
  StateMachineState sm_state_ = StateMachineState::AWAIT_NET_CONNECT;
  DeviceState device_state_;
  TaskHandle_t button_task_h_ = nullptr;
  TaskHandle_t display_task_h_ = nullptr;
  TaskHandle_t network_task_h_ = nullptr;
  TaskHandle_t leds_task_h_ = nullptr;
  TaskHandle_t main_task_h_ = nullptr;

  std::array<Button, NUM_BUTTONS> buttons_;
  LEDs leds_;
  Network network_;
  Display display_;
  MQTTHelper mqtt_;
  HardwareDefinition hw_;
};

#endif  // HOMEBUTTONS_APP_H

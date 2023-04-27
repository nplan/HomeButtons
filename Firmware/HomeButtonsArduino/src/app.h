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
#include "mdi_helper.h"

class App;

enum class BootCause { RESET, TIMER, BUTTON };

namespace AppSMStates {

class InitState : public State<App> {
 public:
  using State<App>::State;

  void entry() override;

  const char* get_name() override { return "InitState"; }
};

class AwakeModeIdleState : public State<App> {
 public:
  using State<App>::State;

  void entry() override;
  void loop() override;

  const char* get_name() override { return "AwakeModeIdleState"; }
};

class UserInputFinishState : public State<App> {
 public:
  using State<App>::State;

  void loop() override;

  const char* get_name() override { return "UserInputFinishState"; }
};

class NetConnectingState : public State<App> {
 public:
  using State<App>::State;

  void loop() override;

  const char* get_name() override { return "NetConnectingState"; }
};

class SettingsMenuState : public State<App> {
 public:
  using State<App>::State;

  void entry() override;
  void loop() override;

  const char* get_name() override { return "SettingsMenuState"; }
};

class CmdShutdownState : public State<App> {
 public:
  using State<App>::State;

  void entry() override;

  const char* get_name() override { return "CmdShutdownState"; }
};

class NetDisconnectingState : public State<App> {
 public:
  using State<App>::State;

  void loop() override;

  const char* get_name() override { return "NetDisconnectingState"; }
};

class ShuttingDownState : public State<App> {
 public:
  using State<App>::State;

  void loop() override;

  const char* get_name() override { return "ShuttingDownState"; }
};

class FactoryResetState : public State<App> {
 public:
  using State<App>::State;

  void entry() override;
  void loop() override;

  const char* get_name() override { return "FactoryResetState"; }
};

}  // namespace AppSMStates

using AppStateMachine = StateMachine<
    App, AppSMStates::InitState, AppSMStates::AwakeModeIdleState,
    AppSMStates::UserInputFinishState, AppSMStates::NetConnectingState,
    AppSMStates::SettingsMenuState, AppSMStates::CmdShutdownState,
    AppSMStates::NetDisconnectingState, AppSMStates::ShuttingDownState,
    AppSMStates::FactoryResetState>;

class App : public AppStateMachine, public Logger {
 public:
  App();
  App(const App&) = delete;
  void setup();

 private:
  void _start_esp_sleep();
  void _go_to_sleep();
  std::pair<BootCause, int16_t> _determine_boot_cause();
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
  void _download_mdi_icons();

  DeviceState device_state_;
  TaskHandle_t button_task_h_ = nullptr;
  TaskHandle_t display_task_h_ = nullptr;
  TaskHandle_t network_task_h_ = nullptr;
  TaskHandle_t leds_task_h_ = nullptr;
  TaskHandle_t main_task_h_ = nullptr;

  LEDs leds_;
  Network network_;
  Display display_;
  MQTTHelper mqtt_;
  HardwareDefinition hw_;
  MDIHelper mdi_;
  ButtonHandler<NUM_BUTTONS> button_handler_;
  ButtonEvent btn_event_;
  BootCause boot_cause_;

  uint32_t last_sensor_publish_ = 0;
  uint32_t last_m_display_redraw_ = 0;
  uint32_t info_screen_start_time_ = 0;
  uint32_t settings_menu_start_time_ = 0;

  friend class AppSMStates::InitState;
  friend class AppSMStates::AwakeModeIdleState;
  friend class AppSMStates::UserInputFinishState;
  friend class AppSMStates::NetConnectingState;
  friend class AppSMStates::SettingsMenuState;
  friend class AppSMStates::CmdShutdownState;
  friend class AppSMStates::NetDisconnectingState;
  friend class AppSMStates::ShuttingDownState;
  friend class AppSMStates::FactoryResetState;
};

#endif  // HOMEBUTTONS_APP_H

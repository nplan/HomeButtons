#ifndef HOMEBUTTONS_APP_H
#define HOMEBUTTONS_APP_H

#include <array>
#include "state.h"
#include "network.h"
#include "display.h"
#include "mqtt_helper.h"
#include "logger.h"
#include "hardware.h"
#include "mdi_helper.h"

#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
#include "original_mini/buttons.h"
#include "original_mini/leds.h"
#elif defined(HOME_BUTTONS_PRO)
#include "pro/touch.h"
#endif

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
  void handle_ui_event(UserInput::Event event);

  const char* get_name() override { return "AwakeModeIdleState"; }
};

class SleepModeHandleInput : public State<App> {
 public:
  using State<App>::State;

  void entry() override;
  void loop() override;
  void handle_ui_event(UserInput::Event event);

  const char* get_name() override { return "SleepModeHandleInput"; }
};

class NetConnectingState : public State<App> {
 public:
  using State<App>::State;

  void entry() override;
  void loop() override;
  void handle_ui_event(UserInput::Event event);

  const char* get_name() override { return "NetConnectingState"; }
};

class InfoScreenState : public State<App> {
 public:
  using State<App>::State;

  void entry() override;
  void exit() override;
  void loop() override;
  void handle_ui_event(UserInput::Event event);

  const char* get_name() override { return "InfoScreenState"; }
};

class SettingsMenuState : public State<App> {
 public:
  using State<App>::State;

  void entry() override;
  void exit() override;
  void loop() override;
  void handle_ui_event(UserInput::Event event);

  const char* get_name() override { return "SettingsMenuState"; }
};

class DeviceInfoState : public State<App> {
 public:
  using State<App>::State;

  void entry() override;
  void loop() override;
  void handle_ui_event(UserInput::Event event);

  const char* get_name() override { return "DeviceInfoState"; }
};

class CmdShutdownState : public State<App> {
 public:
  using State<App>::State;

  void entry() override;
  void loop() override;

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
    AppSMStates::SleepModeHandleInput, AppSMStates::NetConnectingState,
    AppSMStates::InfoScreenState, AppSMStates::SettingsMenuState,
    AppSMStates::DeviceInfoState, AppSMStates::CmdShutdownState,
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
  void _log_task_stats();

  void _begin_buttons();
  void _end_buttons();

  static void _ui_task(void* app);
  static void _display_task(void* app);
  static void _network_task(void* app);

  void _start_ui_task();
  void _start_display_task();
  void _start_network_task();

  void _main_task();
  static void _main_task_helper(void* app) {
    static_cast<App*>(app)->_main_task();
  }

  void _handle_ui_event_global(UserInput::Event event);
  void _publish_btn_event(UserInput::Event event);
  void _publish_sensors();
  void _mqtt_callback(const char* topic, const char* payload);
  void _net_on_connect();
  void _download_mdi_icons();

#if defined(HOME_BUTTONS_ORIGINAL)
  void _publish_awake_mode_avlb();
#endif

  DeviceState device_state_;
  TaskHandle_t ui_task_h_ = nullptr;
  TaskHandle_t display_task_h_ = nullptr;
  TaskHandle_t network_task_h_ = nullptr;
  TaskHandle_t main_task_h_ = nullptr;

#if defined(HOME_BUTTONS_ORIGINAL)
  Button btn1_;
  Button btn2_;
  Button btn3_;
  Button btn4_;
  Button btn5_;
  Button btn6_;
  ButtonInput<NUM_BUTTONS> buttons_;
  LED led1_;
  LED led2_;
  LED led3_;
  LED led4_;
  LED led5_;
  LED led6_;
  LEDs<NUM_BUTTONS> leds_;
#elif defined(HOME_BUTTONS_MINI)
  Button btn1_;
  Button btn2_;
  Button btn3_;
  Button btn4_;
  ButtonInput<NUM_BUTTONS> buttons_;
  LED led1_;
  LED led2_;
  LED led3_;
  LED led4_;
  LEDs<NUM_BUTTONS> leds_;
#elif defined(HOME_BUTTONS_PRO)
  TouchInput touch_handler_;
#endif
  UserInput::Event user_event_ = {};

  Network network_;
  Display display_;
  MQTTHelper mqtt_;
  HardwareDefinition hw_;
  MDIHelper mdi_;

  BootCause boot_cause_;
  uint8_t wakeup_btn_id_ = 0;

  uint32_t last_sensor_publish_ = 0;
  uint32_t last_m_display_redraw_ = 0;
  uint32_t input_start_time_ = 0;
  uint32_t info_screen_start_time_ = 0;
  uint32_t settings_menu_start_time_ = 0;
  uint32_t device_info_start_time_ = 0;
  uint32_t shutdown_cmd_time_ = 0;

  friend class AppSMStates::InitState;
  friend class AppSMStates::AwakeModeIdleState;
  friend class AppSMStates::SleepModeHandleInput;
  friend class AppSMStates::NetConnectingState;
  friend class AppSMStates::InfoScreenState;
  friend class AppSMStates::SettingsMenuState;
  friend class AppSMStates::DeviceInfoState;
  friend class AppSMStates::CmdShutdownState;
  friend class AppSMStates::NetDisconnectingState;
  friend class AppSMStates::ShuttingDownState;
  friend class AppSMStates::FactoryResetState;
};

#endif  // HOMEBUTTONS_APP_H

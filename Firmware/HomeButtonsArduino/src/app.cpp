#include "app.h"

#include <Arduino.h>
#include <esp_task_wdt.h>

#include "config.h"
#include "factory.h"
#include "setup.h"
#include "hardware.h"

App::App()
    : Logger("APP"),
      network_(device_state_),
      display_(device_state_),
      mqtt_(device_state_, network_) {}

void App::setup() {
  xTaskCreate(_main_task_helper,  // Function that should be called
              "MAIN",             // Name of the task (for debugging)
              20000,              // Stack size (bytes)
              this,               // Parameter to pass
              tskIDLE_PRIORITY,   // Task priority
              &main_task_h_       // Task handle
  );
  debug("main task started.");

  vTaskDelete(NULL);
}

void App::_start_esp_sleep() {
  esp_sleep_enable_ext1_wakeup(hw_.WAKE_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
  if (device_state_.persisted().wifi_done &&
      device_state_.persisted().setup_done &&
      !device_state_.persisted().low_batt_mode &&
      !device_state_.persisted().check_connection) {
    if (device_state_.persisted().info_screen_showing) {
      esp_sleep_enable_timer_wakeup(INFO_SCREEN_DISP_TIME * 1000UL);
    } else {
      esp_sleep_enable_timer_wakeup(device_state_.sensor_interval() *
                                    60000000UL);
    }
  }
  info("deep sleep... z z z");
  esp_deep_sleep_start();
}

void App::_go_to_sleep() {
  device_state_.save_all();
  hw_.set_all_leds(0);
  _start_esp_sleep();
}

std::pair<BootCause, Button*> App::_determine_boot_cause() {
  BootCause boot_cause = BootCause::RESET;
  int16_t wakeup_pin = 0;
  Button* active_button = nullptr;
  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_EXT1: {
      uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
      wakeup_pin = (log(GPIO_reason)) / log(2);
      info("wakeup cause: PIN %d", wakeup_pin);
      if (wakeup_pin == hw_.BTN1_PIN || wakeup_pin == hw_.BTN2_PIN ||
          wakeup_pin == hw_.BTN3_PIN || wakeup_pin == hw_.BTN4_PIN ||
          wakeup_pin == hw_.BTN5_PIN || wakeup_pin == hw_.BTN6_PIN) {
        boot_cause = BootCause::BUTTON;
      } else {
        boot_cause = BootCause::RESET;
      }
    } break;
    case ESP_SLEEP_WAKEUP_TIMER:
      info("wakeup cause: TIMER");
      boot_cause = BootCause::TIMER;
      break;
    default:
      info("wakeup cause: RESET");
      boot_cause = BootCause::RESET;
      break;
  }
  for (auto& b : buttons_) {
    if (b.get_pin() == wakeup_pin) {
      active_button = &b;
    }
  }
  return std::make_pair(boot_cause, active_button);
}

void App::_log_stack_status() const {
  uint32_t btns_free = uxTaskGetStackHighWaterMark(button_task_h_);
  uint32_t disp_free = uxTaskGetStackHighWaterMark(display_task_h_);
  uint32_t net_free = uxTaskGetStackHighWaterMark(network_task_h_);
  uint32_t leds_free = uxTaskGetStackHighWaterMark(leds_task_h_);
  uint32_t main_free = uxTaskGetStackHighWaterMark(main_task_h_);
  uint32_t num_tasks = uxTaskGetNumberOfTasks();
  info(
      "free stack: btns %d, disp %d, net %d, leds %d, main "
      "%d, num tasks %d",
      btns_free, disp_free, net_free, leds_free, main_free, num_tasks);
  uint32_t esp_free_heap = ESP.getFreeHeap();
  uint32_t esp_min_free_heap = ESP.getMinFreeHeap();
  uint32_t rtos_free_heap = xPortGetFreeHeapSize();
  info("free heap: esp %d, esp min %d, rtos %d", esp_free_heap,
       esp_min_free_heap, rtos_free_heap);
}

void App::_begin_buttons() {
  std::pair<uint8_t, uint16_t> button_map[NUM_BUTTONS] = {
      {hw_.BTN1_PIN, 1}, {hw_.BTN6_PIN, 2}, {hw_.BTN2_PIN, 3},
      {hw_.BTN5_PIN, 4}, {hw_.BTN3_PIN, 5}, {hw_.BTN4_PIN, 6}};
  for (uint i = 0; i < NUM_BUTTONS; i++) {
    buttons_[i].begin(button_map[i].first, button_map[i].second);
  }
}

void App::_end_buttons() {
  for (auto& b : buttons_) {
    b.end();
  }
}

void App::_button_task(void* param) {
  App* app = static_cast<App*>(param);
  while (true) {
    for (auto& b : app->buttons_) {
      b.update();
    }
    delay(20);
  }
}

void App::_leds_task(void* param) {
  App* app = static_cast<App*>(param);
  while (true) {
    app->leds_.update(app->hw_);
    delay(100);
  }
}

void App::_display_task(void* param) {
  App* app = static_cast<App*>(param);
  while (true) {
    app->display_.update();
    delay(50);
  }
}

void App::_network_task(void* param) {
  App* app = static_cast<App*>(param);
  app->network_.setup();
  while (true) {
    app->network_.update();
    delay(10);
  }
}

void App::_start_button_task() {
  if (button_task_h_ != nullptr) return;
  debug("button task started.");
  xTaskCreate(_button_task,    // Function that should be called
              "BUTTON",        // Name of the task (for debugging)
              2000,            // Stack size (bytes)
              this,            // Parameter to pass
              1,               // Task priority
              &button_task_h_  // Task handle
  );
}

void App::_start_display_task() {
  if (display_task_h_ != nullptr) return;
  debug("m_display task started.");
  xTaskCreate(_display_task,    // Function that should be called
              "DISPLAY",        // Name of the task (for debugging)
              5000,             // Stack size (bytes)
              this,             // Parameter to pass
              1,                // Task priority
              &display_task_h_  // Task handle
  );
}

void App::_start_network_task() {
  if (network_task_h_ != nullptr) return;
  debug("network task started.");
  xTaskCreate(_network_task,    // Function that should be called
              "NETWORK",        // Name of the task (for debugging)
              20000,            // Stack size (bytes)
              this,             // Parameter to pass
              1,                // Task priority
              &network_task_h_  // Task handle
  );
}

void App::_start_leds_task() {
  if (leds_task_h_ != nullptr) return;
  debug("leds task started.");
  xTaskCreate(&_leds_task,   // Function that should be called
              "LEDS",        // Name of the task (for debugging)
              2000,          // Stack size (bytes)
              this,          // Parameter to pass
              1,             // Task priority
              &leds_task_h_  // Task handle
  );
}

void App::_main_task() {
  info("woke up.");
  info("SW version: %s", SW_VERSION);
  device_state_.load_all();

  // ------ factory mode ------
  if (device_state_.factory().serial_number.length() < 1) {
    info("first boot, starting factory mode...");
    factory::factory_mode(hw_, device_state_, display_);
    display_.begin(hw_);
    display_.disp_welcome();
    display_.update();
    display_.end();
    info("factory settings complete. Going to sleep.");
    _start_esp_sleep();
  }

  // ------ init hardware ------
  info("HW version: %s", device_state_.factory().hw_version.c_str());
  hw_.init(device_state_.factory().hw_version);
  _begin_buttons();
  display_.begin(
      hw_);  // must be before ledAttachPin (reserves GPIO37 = SPIDQS)
  hw_.begin();

  // ------ after update handler ------
  if (device_state_.persisted().last_sw_ver != SW_VERSION) {
    if (device_state_.persisted().last_sw_ver.length() > 0) {
      info("firmware updated from %s to %s",
           device_state_.persisted().last_sw_ver.c_str(), SW_VERSION);
      device_state_.persisted().last_sw_ver = SW_VERSION;
      device_state_.persisted().send_discovery_config = true;
      device_state_.save_all();
      display_.disp_message(
          (UIState::MessageType("Firmware\nupdated to\n") + SW_VERSION)
              .c_str());
      display_.update();
      ESP.restart();
    } else {  // first boot after factory flash
      device_state_.persisted().last_sw_ver = SW_VERSION;
    }
  }

  // ------ determine power mode ------
  device_state_.sensors().battery_present = hw_.is_battery_present();
  device_state_.sensors().dc_connected = hw_.is_dc_connected();
  info("batt present: %d, DC connected: %d",
       device_state_.sensors().battery_present,
       device_state_.sensors().dc_connected);

  if (device_state_.sensors().battery_present) {
    float batt_voltage = hw_.read_battery_voltage();
    info("batt volts: %f", batt_voltage);
    if (device_state_.sensors().dc_connected) {
      // charging
      if (batt_voltage < hw_.CHARGE_HYSTERESIS_VOLT) {
        device_state_.sensors().charging = true;
        hw_.enable_charger(true);
        device_state_.flags().awake_mode = true;
      } else {
        device_state_.flags().awake_mode =
            device_state_.persisted().user_awake_mode;
      }
      device_state_.persisted().low_batt_mode = false;
    } else {  // dc_connected == false
      if (device_state_.persisted().low_batt_mode) {
        if (batt_voltage >= hw_.BATT_HYSTERESIS_VOLT) {
          device_state_.persisted().low_batt_mode = false;
          device_state_.save_all();
          info("low batt mode disabled");
          ESP.restart();  // to handle m_display update
        } else {
          info("in low batt mode...");
          _go_to_sleep();
        }
      } else {  // low_batt_mode == false
        if (batt_voltage < hw_.MIN_BATT_VOLT) {
          // check again
          delay(1000);
          batt_voltage = hw_.read_battery_voltage();
          if (batt_voltage < hw_.MIN_BATT_VOLT) {
            device_state_.persisted().low_batt_mode = true;
            warning("batt voltage too low, low bat mode enabled");
            display_.disp_message_large(
                "Turned\nOFF\n\nPlease\nrecharge\nbattery!");
            display_.update();
            _go_to_sleep();
          }
        } else if (batt_voltage <= hw_.WARN_BATT_VOLT) {
          device_state_.sensors().battery_low = true;
        }
        device_state_.flags().awake_mode = false;
      }
    }
  } else {  // battery_present == false
    if (device_state_.sensors().dc_connected) {
      device_state_.persisted().low_batt_mode = false;
      // choose power mode based on user setting
      device_state_.flags().awake_mode =
          device_state_.persisted().user_awake_mode;
    } else {
      // should never happen
      _go_to_sleep();
    }
  }
  info("usr awake mode: %d, awake mode: %d",
       device_state_.persisted().user_awake_mode,
       device_state_.flags().awake_mode);

  // ------ read sensors ------
  hw_.read_temp_hmd(device_state_.sensors().temperature,
                    device_state_.sensors().humidity);
  device_state_.sensors().battery_pct = hw_.read_battery_percent();

  // ------ boot cause ------
  auto&& [boot_cause, active_button] = _determine_boot_cause();

  // ------ handle boot cause ------
  switch (boot_cause) {
    case BootCause::RESET: {
      if (!device_state_.persisted().silent_restart) {
        display_.disp_message("RESTART...", 0);
        display_.update();
      }

      if (device_state_.persisted().restart_to_wifi_setup) {
        device_state_.clear_persisted_flags();
        info("staring Wi-Fi setup...");
        start_wifi_setup(device_state_, display_);  // resets ESP when done
      } else if (device_state_.persisted().restart_to_setup) {
        device_state_.clear_persisted_flags();
        info("staring setup...");
        start_setup(device_state_, display_, hw_);  // resets ESP when done
      }
      device_state_.clear_persisted_flags();
      if (!device_state_.persisted().wifi_done ||
          !device_state_.persisted().setup_done) {
        display_.disp_welcome();
        display_.update();
        display_.end();
        display_.update();
        _go_to_sleep();
      } else {
        display_.disp_main();
        display_.update();
      }
      if (device_state_.flags().awake_mode) {
        // proceed with awake mode
        sm_state_ = StateMachineState::AWAIT_NET_CONNECT;
      } else {
        display_.end();
        display_.update();
        _go_to_sleep();
      }
      break;
    }
    case BootCause::TIMER: {
      if (device_state_.flags().awake_mode) {
        // proceed with awake mode
        sm_state_ = StateMachineState::AWAIT_NET_CONNECT;
      } else {
        if (device_state_.persisted().info_screen_showing) {
          device_state_.persisted().info_screen_showing = false;
          display_.disp_main();
        }
        if (hw_.is_charger_in_standby()) {  // hw <= 2.1 doesn't have awake
          // mode when charging
          if (!device_state_.persisted().charge_complete_showing) {
            device_state_.persisted().charge_complete_showing = true;
            display_.disp_message_large("Fully\ncharged!");
          }
        }
        // proceed with sensor publish
        sm_state_ = StateMachineState::AWAIT_NET_CONNECT;
      }
      break;
    }
    case BootCause::BUTTON: {
      if (!device_state_.persisted().wifi_done) {
        start_wifi_setup(device_state_, display_);
      } else if (!device_state_.persisted().setup_done) {
        start_setup(device_state_, display_, hw_);
      }

      if (!device_state_.flags().awake_mode) {
        if (device_state_.persisted().info_screen_showing) {
          device_state_.persisted().info_screen_showing = false;
          display_.disp_main();
          display_.update();
          display_.end();
          display_.update();
          _go_to_sleep();
        } else if (device_state_.persisted().charge_complete_showing) {
          device_state_.persisted().charge_complete_showing = false;
          display_.disp_main();
          display_.update();
          display_.end();
          display_.update();
          _go_to_sleep();
        } else if (device_state_.persisted().check_connection) {
          device_state_.persisted().check_connection = false;
          display_.disp_main();
          display_.update();
          display_.end();
          display_.update();
          _go_to_sleep();
        } else {
          info("Active button : %p", active_button);
          if (active_button != nullptr) {
            active_button->init_press();
            _start_button_task();
            leds_.begin();
            _start_leds_task();
            sm_state_ = StateMachineState::AWAIT_USER_INPUT_FINISH;
          } else {
            _go_to_sleep();
          }
        }
      } else {
        // proceed with awake mode
        sm_state_ = StateMachineState::AWAIT_NET_CONNECT;
      }
      break;
    }
  }

  display_.init_ui_state(UIState{.page = DisplayPage::MAIN});
  network_.set_mqtt_callback(std::bind(&App::_mqtt_callback, this,
                                       std::placeholders::_1,
                                       std::placeholders::_2));
  network_.set_on_connect(std::bind(&App::_net_on_connect, this));

  if (!device_state_.flags().awake_mode) {
    // ######## SLEEP MODE state machine ########
    esp_task_wdt_init(WDT_TIMEOUT_SLEEP, true);
    esp_task_wdt_add(NULL);
    Button::ButtonAction btn_action = Button::IDLE;
    Button::ButtonAction prev_action = Button::IDLE;
    _start_display_task();
    _start_network_task();
    network_.connect();
    while (true) {
      switch (sm_state_) {
        case StateMachineState::AWAIT_USER_INPUT_FINISH: {
          if (active_button == nullptr) {
            error("active button = null");
            sm_state_ = StateMachineState::CMD_SHUTDOWN;
          } else if (active_button->is_press_finished()) {
            btn_action = active_button->get_action();
            debug("BTN_%d pressed - state %s", active_button->get_id(),
                  active_button->get_action_name(btn_action));
            switch (btn_action) {
              case Button::SINGLE:
              case Button::DOUBLE:
              case Button::TRIPLE:
              case Button::QUAD:
                leds_.blink(active_button->get_id(),
                            Button::get_action_multi_count(btn_action), true);
                if (device_state_.sensors().battery_low) {
                  display_.disp_message_large(
                      "Battery\nLOW\n\nPlease\nrecharge\nsoon!", 3000);
                }
                sm_state_ = StateMachineState::AWAIT_NET_CONNECT;
                break;
              case Button::LONG_1:
                // info screen - already m_displayed by transient action handler
                device_state_.persisted().info_screen_showing = true;
                debug("m_displayed info screen");
                sm_state_ = StateMachineState::CMD_SHUTDOWN;
                break;
              case Button::LONG_2:
                device_state_.persisted().restart_to_setup = true;
                device_state_.save_all();
                ESP.restart();
                break;
              case Button::LONG_3:
                device_state_.persisted().restart_to_wifi_setup = true;
                device_state_.save_all();
                ESP.restart();
                break;
              case Button::LONG_4:
                // factory reset
                display_.disp_message("Factory\nRESET...");
                network_.disconnect(true);  // erase login data
                display_.end();
                _end_buttons();
                sm_state_ = StateMachineState::AWAIT_FACTORY_RESET;
                break;
              default:
                sm_state_ = StateMachineState::CMD_SHUTDOWN;
                break;
            }
            for (auto& b : buttons_) {
              b.clear();
            }
          } else {
            Button::ButtonAction new_action = active_button->get_action();
            if (new_action != prev_action) {
              switch (new_action) {
                case Button::LONG_1:
                  // info screen
                  display_.disp_info();
                  break;
                case Button::LONG_2:
                  display_.disp_message(
                      "Release\nfor\nSETUP\n\nKeep\nholding\nfor\nWi-Fi "
                      "SETUP");
                  break;
                case Button::LONG_3:
                  display_.disp_message(
                      "Release\nfor\nWi-Fi SETUP\n\nKeep\n"
                      "holding\nfor\nFACTORY\nRESET");
                  break;
                case Button::LONG_4:
                  display_.disp_message("Release\nfor\nFACTORY\nRESET");
                  break;
                default:
                  break;
              }
              prev_action = new_action;
            }
          }
          break;
        }
        case StateMachineState::AWAIT_NET_CONNECT: {
          if (network_.get_state() == Network::State::M_CONNECTED) {
            // net_on_connect();
            if (active_button != nullptr && btn_action != Button::IDLE) {
              network_.publish(
                  mqtt_.get_button_topic(active_button->get_id(), btn_action),
                  BTN_PRESS_PAYLOAD);
            }
            hw_.read_temp_hmd(device_state_.sensors().temperature,
                              device_state_.sensors().humidity);
            device_state_.sensors().battery_pct = hw_.read_battery_percent();
            _publish_sensors();
            device_state_.persisted().failed_connections = 0;
            sm_state_ = StateMachineState::CMD_SHUTDOWN;
          } else if (millis() >= NET_CONNECT_TIMEOUT) {
            debug("network connect timeout.");
            if (boot_cause == BootCause::BUTTON) {
              display_.disp_error("Network\nconnection\nnot\nsuccessful", 3000);
              delay(100);
            } else if (boot_cause == BootCause::TIMER) {
              device_state_.persisted().failed_connections++;
              if (device_state_.persisted().failed_connections >=
                  MAX_FAILED_CONNECTIONS) {
                device_state_.persisted().failed_connections = 0;
                device_state_.persisted().check_connection = true;
                display_.disp_error("Check\nconnection!");
                delay(100);
              }
            }
            sm_state_ = StateMachineState::CMD_SHUTDOWN;
          }
          break;
        }
        case StateMachineState::CMD_SHUTDOWN: {
          _end_buttons();
          leds_.end();
          network_.disconnect();
          sm_state_ = StateMachineState::AWAIT_NET_DISCONNECT;
          break;
        }
        case StateMachineState::AWAIT_NET_DISCONNECT: {
          if (network_.get_state() == Network::State::DISCONNECTED) {
            if (device_state_.flags().display_redraw) {
              device_state_.flags().display_redraw = false;
              display_.disp_main();
              delay(100);
            }
            display_.end();
            sm_state_ = StateMachineState::AWAIT_SHUTDOWN;
          }
          break;
        }
        case StateMachineState::AWAIT_SHUTDOWN: {
          if (display_.get_state() == Display::State::IDLE &&
              leds_.get_state() == LEDs::State::IDLE) {
            _log_stack_status();
            if (device_state_.flags().awake_mode) {
              device_state_.persisted().silent_restart = true;
              device_state_.save_all();
              ESP.restart();
            } else {
              _go_to_sleep();
            }
          }
          break;
        }
        case StateMachineState::AWAIT_FACTORY_RESET: {
          if (network_.get_state() == Network::State::DISCONNECTED &&
              display_.get_state() == Display::State::IDLE) {
            device_state_.clear_all();
            info("factory reset complete.");
            ESP.restart();
          }
          break;
        }
        default:
          break;
      }  // end switch()
      delay(10);
    }  // end while()
  } else {
    // ######## AWAKE MODE state machine ########
    esp_task_wdt_init(WDT_TIMEOUT_AWAKE, true);
    esp_task_wdt_add(NULL);
    Button::ButtonAction prev_action = Button::IDLE;
    uint32_t last_sensor_publish = 0;
    uint32_t last_m_display_redraw = 0;
    uint32_t info_screen_start_time = 0;
    device_state_.persisted().info_screen_showing = false;
    device_state_.persisted().check_connection = false;
    device_state_.persisted().charge_complete_showing = false;
    display_.disp_main();
    _start_button_task();
    leds_.begin();
    _start_leds_task();
    _start_display_task();
    _start_network_task();
    network_.connect();
    while (true) {
      switch (sm_state_) {
        case StateMachineState::AWAIT_NET_CONNECT: {
          if (network_.get_state() == Network::State::M_CONNECTED) {
            for (auto& b : buttons_) {
              b.clear();
            }
            hw_.read_temp_hmd(device_state_.sensors().temperature,
                              device_state_.sensors().humidity);
            device_state_.sensors().battery_pct = hw_.read_battery_percent();
            _publish_sensors();
            sm_state_ = StateMachineState::AWAIT_USER_INPUT_START;
          } else if (!hw_.is_dc_connected()) {
            device_state_.sensors().charging = false;
            device_state_.persisted().info_screen_showing = false;
            display_.disp_main();
            delay(100);
            sm_state_ = StateMachineState::CMD_SHUTDOWN;
          }
          break;
        }
        case StateMachineState::AWAIT_USER_INPUT_START: {
          for (auto& b : buttons_) {
            if (b.get_action() != Button::IDLE) {
              active_button = &b;
              break;  // for
            }
          }
          if (active_button != nullptr) {
            sm_state_ = StateMachineState::AWAIT_USER_INPUT_FINISH;
          } else if (millis() - last_sensor_publish >= AWAKE_SENSOR_INTERVAL) {
            hw_.read_temp_hmd(device_state_.sensors().temperature,
                              device_state_.sensors().humidity);
            device_state_.sensors().battery_pct = hw_.read_battery_percent();
            _publish_sensors();
            last_sensor_publish = millis();
            // log stack status
            _log_stack_status();
          } else if (millis() - last_m_display_redraw >=
                     AWAKE_REDRAW_INTERVAL) {
            if (device_state_.flags().display_redraw) {
              device_state_.flags().display_redraw = false;
              if (device_state_.persisted().info_screen_showing) {
                display_.disp_info();
              } else {
                display_.disp_main();
              }
            }
            last_m_display_redraw = millis();
          } else if (device_state_.persisted().info_screen_showing &&
                     millis() - info_screen_start_time >=
                         INFO_SCREEN_DISP_TIME) {
            display_.disp_main();
            device_state_.persisted().info_screen_showing = false;
          } else if (!hw_.is_dc_connected()) {
            _publish_awake_mode_avlb();
            device_state_.sensors().charging = false;
            display_.disp_main();
            delay(100);
            sm_state_ = StateMachineState::CMD_SHUTDOWN;
          }
          if (device_state_.sensors().charging) {
            if (hw_.is_charger_in_standby()) {
              device_state_.sensors().charging = false;
              display_.disp_main();
              if (!device_state_.persisted().user_awake_mode) {
                delay(100);
                sm_state_ = StateMachineState::CMD_SHUTDOWN;
              }
            }
          } else {
            if (!device_state_.persisted().user_awake_mode) {
              display_.disp_main();
              delay(100);
              sm_state_ = StateMachineState::CMD_SHUTDOWN;
            }
          }
          break;
        }
        case StateMachineState::AWAIT_USER_INPUT_FINISH: {
          if (active_button == nullptr) {
            for (auto& b : buttons_) {
              b.clear();
            }
            sm_state_ = StateMachineState::AWAIT_USER_INPUT_START;
          } else if (active_button->is_press_finished()) {
            auto btn_action = active_button->get_action();
            debug("BTN_%d pressed - state %s", active_button->get_id(),
                  active_button->get_action_name(btn_action));
            switch (btn_action) {
              case Button::SINGLE:
              case Button::DOUBLE:
              case Button::TRIPLE:
              case Button::QUAD:
                if (device_state_.persisted().info_screen_showing) {
                  display_.disp_main();
                  device_state_.persisted().info_screen_showing = false;
                  sm_state_ = StateMachineState::AWAIT_USER_INPUT_START;
                  break;
                }
                leds_.blink(active_button->get_id(),
                            Button::get_action_multi_count(btn_action));
                network_.publish(
                    mqtt_.get_button_topic(active_button->get_id(), btn_action),
                    BTN_PRESS_PAYLOAD);
                sm_state_ = StateMachineState::AWAIT_USER_INPUT_START;
                break;
              case Button::LONG_1:
                if (device_state_.persisted().info_screen_showing) {
                  display_.disp_main();
                  device_state_.persisted().info_screen_showing = false;
                  sm_state_ = StateMachineState::AWAIT_USER_INPUT_START;
                  break;
                }
                // info screen - already m_displayed by transient action handler
                device_state_.persisted().info_screen_showing = true;
                info_screen_start_time = millis();
                debug("m_displayed info screen");
                sm_state_ = StateMachineState::AWAIT_USER_INPUT_START;
                break;
              case Button::LONG_2:
                device_state_.persisted().restart_to_setup = true;
                device_state_.save_all();
                ESP.restart();
                break;
              case Button::LONG_3:
                device_state_.persisted().restart_to_wifi_setup = true;
                device_state_.save_all();
                ESP.restart();
                break;
              case Button::LONG_4:
                // factory reset
                display_.disp_message("Factory\nRESET...");
                network_.disconnect(true);  // erase login data
                display_.end();
                _end_buttons();
                sm_state_ = StateMachineState::AWAIT_FACTORY_RESET;
                break;
              default:
                sm_state_ = StateMachineState::AWAIT_USER_INPUT_START;
                break;
            }
            for (auto& b : buttons_) {
              b.clear();
            }
            active_button = nullptr;
          } else {
            auto new_action = active_button->get_action();
            if (new_action != prev_action) {
              switch (new_action) {
                case Button::LONG_1:
                  // info screen
                  display_.disp_info();
                  break;
                case Button::LONG_2:
                  display_.disp_message(
                      "Release\nfor\nSETUP\n\nKeep\nholding\nfor\nWi-Fi "
                      "SETUP");
                  break;
                case Button::LONG_3:
                  display_.disp_message(
                      "Release\nfor\nWi-Fi SETUP\n\nKeep\n"
                      "holding\nfor\nFACTORY\nRESET");
                  break;
                case Button::LONG_4:
                  display_.disp_message("Release\nfor\nFACTORY\nRESET");
                  break;
                default:
                  break;
              }
              prev_action = new_action;
            }
          }
          break;
        }
        case StateMachineState::CMD_SHUTDOWN: {
          device_state_.persisted().info_screen_showing = false;
          _end_buttons();
          leds_.end();
          network_.disconnect();
          sm_state_ = StateMachineState::AWAIT_NET_DISCONNECT;
          break;
        }
        case StateMachineState::AWAIT_NET_DISCONNECT: {
          if (network_.get_state() == Network::State::DISCONNECTED) {
            if (device_state_.flags().display_redraw) {
              device_state_.flags().display_redraw = false;
              display_.disp_main();
              delay(100);
            }
            display_.end();
            sm_state_ = StateMachineState::AWAIT_SHUTDOWN;
          }
          break;
        }
        case StateMachineState::AWAIT_SHUTDOWN: {
          if (display_.get_state() == Display::State::IDLE &&
              leds_.get_state() == LEDs::State::IDLE) {
            _go_to_sleep();
          }
          break;
        }
        case StateMachineState::AWAIT_FACTORY_RESET: {
          if (network_.get_state() == Network::State::DISCONNECTED &&
              display_.get_state() == Display::State::IDLE) {
            device_state_.clear_all();
            info("factory reset complete.");
            ESP.restart();
          }
          break;
        }
      }  // end switch()
      if (ESP.getMinFreeHeap() < MIN_FREE_HEAP) {
        warning("free heap low, restarting...");
        _log_stack_status();
        device_state_.persisted().silent_restart = true;
        device_state_.save_all();
        ESP.restart();
      }
      esp_task_wdt_reset();
      delay(10);
    }  // end while()
  }    // end else
}

void App::_publish_sensors() {
  network_.publish(mqtt_.t_temperature(),
                   PayloadType("%.2f", device_state_.sensors().temperature));
  network_.publish(mqtt_.t_humidity(),
                   PayloadType("%.2f", device_state_.sensors().humidity));
  network_.publish(mqtt_.t_battery(),
                   PayloadType("%u", device_state_.sensors().battery_pct));
}

void App::_publish_awake_mode_avlb() {
  if (hw_.is_dc_connected()) {
    network_.publish(mqtt_.t_awake_mode_avlb(), "online", true);
  } else {
    network_.publish(mqtt_.t_awake_mode_avlb(), "offline", true);
  }
}

void App::_mqtt_callback(const char* topic, const char* payload) {
  if (strcmp(topic, mqtt_.t_sensor_interval_cmd().c_str()) == 0) {
    uint16_t mins = atoi(payload);
    if (mins >= SEN_INTERVAL_MIN && mins <= SEN_INTERVAL_MAX) {
      device_state_.set_sensor_interval(mins);
      device_state_.save_all();
      network_.publish(mqtt_.t_sensor_interval_state(),
                       PayloadType("%u", device_state_.sensor_interval()),
                       true);
      mqtt_.update_discovery_config();
      debug("sensor interval set to %d minutes", mins);
      _publish_sensors();
    }
    network_.publish(mqtt_.t_sensor_interval_cmd(), "", true);
    return;
  }

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    if (strcmp(topic, mqtt_.t_btn_label_cmd(i).c_str()) == 0) {
      device_state_.set_btn_label(i, payload);
      debug("button %d label changed to: %s", i + 1,
            device_state_.get_btn_label(i).c_str());
      network_.publish(mqtt_.t_btn_label_state(i),
                       device_state_.get_btn_label(i), true);
      network_.publish(mqtt_.t_btn_label_cmd(i), "", true);
      device_state_.flags().display_redraw = true;
      return;
    }
  }

  if (strcmp(topic, mqtt_.t_awake_mode_cmd().c_str()) == 0) {
    if (strcmp(payload, "ON") == 0) {
      device_state_.persisted().user_awake_mode = true;
      device_state_.flags().awake_mode = true;
      device_state_.save_all();
      network_.publish(mqtt_.t_awake_mode_state(), "ON", true);
      debug("user awake mode set to: ON");
      debug("resetting to awake mode...");
    } else if (strcmp(payload, "OFF") == 0) {
      device_state_.persisted().user_awake_mode = false;
      device_state_.save_all();
      network_.publish(mqtt_.t_awake_mode_state(), "OFF", true);
      debug("user awake mode set to: OFF");
    }
    network_.publish(mqtt_.t_awake_mode_cmd(), "", true);
    return;
  }
}

void App::_net_on_connect() {
  network_.subscribe(mqtt_.t_cmd() + "#");
  delay(100);
  _publish_awake_mode_avlb();
  network_.publish(mqtt_.t_sensor_interval_state(),
                   PayloadType("%u", device_state_.sensor_interval()), true);
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    auto t = mqtt_.t_btn_label_state(i);
    network_.publish(t, device_state_.get_btn_label(i), true);
  }
  network_.publish(mqtt_.t_awake_mode_state(),
                   (device_state_.persisted().user_awake_mode) ? "ON" : "OFF",
                   true);

  if (device_state_.persisted().send_discovery_config) {
    device_state_.persisted().send_discovery_config = false;
    mqtt_.send_discovery_config();
  }
}

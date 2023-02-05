#include <Arduino.h>
#include <WiFiManager.h>
#include <esp_task_wdt.h>

#include "autodiscovery.h"
#include "buttons.h"
#include "config.h"
#include "display.h"
#include "factory.h"
#include "hardware.h"
#include "hw_tests.h"
#include "leds.h"
#include "network.h"
#include "setup.h"
#include "state.h"
#include "memory.h"

enum class BootCause { RESET, TIMER, BUTTON };

const uint8_t num_buttons = 6;
Button btn_1, btn_2, btn_3, btn_4, btn_5, btn_6;
Button *buttons[] = {&btn_1, &btn_2, &btn_3, &btn_4, &btn_5, &btn_6};

enum class StateMachineState {
  AWAIT_NET_CONNECT,
  AWAIT_USER_INPUT_START,
  AWAIT_USER_INPUT_FINISH,
  CMD_SHUTDOWN,
  AWAIT_NET_DISCONNECT,
  AWAIT_SHUTDOWN,
  AWAIT_FACTORY_RESET,
};
StateMachineState sm_state;

TaskHandle_t button_task_h = nullptr;
TaskHandle_t display_task_h = nullptr;
TaskHandle_t network_task_h = nullptr;
TaskHandle_t leds_task_h = nullptr;
TaskHandle_t main_task_h = nullptr;

void start_esp_sleep() {
  esp_sleep_enable_ext1_wakeup(HW.WAKE_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
  if (device_state.wifi_done && device_state.setup_done &&
      !device_state.low_batt_mode && !device_state.check_connection) {
    if (device_state.info_screen_showing) {
      esp_sleep_enable_timer_wakeup(INFO_SCREEN_DISP_TIME * 1000UL);
    } else {
      esp_sleep_enable_timer_wakeup(device_state.sensor_interval() * 60000000UL);
    }
  }
  log_i("[DEVICE] deep sleep... z z z");
  esp_deep_sleep_start();
}

void go_to_sleep() {
  device_state.save_all();
  HW.set_all_leds(0);
  start_esp_sleep();
}

void begin_buttons() {
  btn_1.begin(HW.BTN1_PIN, 1);
  btn_2.begin(HW.BTN6_PIN, 2);
  btn_3.begin(HW.BTN2_PIN, 3);
  btn_4.begin(HW.BTN5_PIN, 4);
  btn_5.begin(HW.BTN3_PIN, 5);
  btn_6.begin(HW.BTN4_PIN, 6);
}

void end_buttons() {
  btn_1.end();
  btn_2.end();
  btn_3.end();
  btn_4.end();
  btn_5.end();
  btn_6.end();
}

void publish_sensors() {
  network.publish(device_state.topics().t_temperature.c_str(),
                  String(device_state.temperature).c_str());
  network.publish(device_state.topics().t_humidity.c_str(),
                  String(device_state.humidity).c_str());
  network.publish(device_state.topics().t_battery.c_str(),
                  String(device_state.battery_pct).c_str());
}

void publish_awake_mode_avlb() {
  if (HW.is_dc_connected()) {
    network.publish(device_state.topics().t_awake_mode_avlb.c_str(), "online", true);
  } else {
    network.publish(device_state.topics().t_awake_mode_avlb.c_str(), "offline", true);
  }
}

void mqtt_callback(const String& topic, const String& payload) {
  if (topic == device_state.topics().t_sensor_interval_cmd) {
    uint16_t mins = payload.toInt();
    if (mins >= SEN_INTERVAL_MIN && mins <= SEN_INTERVAL_MAX) {
      device_state.set_sensor_interval(mins);
      device_state.save_all();
      network.publish(device_state.topics().t_sensor_interval_state.c_str(),
                      String(device_state.sensor_interval()).c_str(), true);
      update_discovery_config();
      log_d("[DEVICE] sensor interval set to %d minutes", mins);
      publish_sensors();
    }
    network.publish(device_state.topics().t_sensor_interval_cmd.c_str(), nullptr, true);
  }

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    if (topic == device_state.topics().t_btn_label_cmd[i]) {
      device_state.set_btn_label(i, payload.c_str());
      log_d("[DEVICE] button %d label changed to: %s", i + 1,
            device_state.get_btn_label(i));
      network.publish(device_state.topics().t_btn_label_state[i].c_str(),
                      device_state.get_btn_label(i), true);
      network.publish(device_state.topics().t_btn_label_cmd[i].c_str(), nullptr, true);
      device_state.display_redraw = true;
    }
  }

  if (topic == device_state.topics().t_awake_mode_cmd) {
    if (payload == "ON") {
      device_state.user_awake_mode = true;
      device_state.awake_mode = true;
      device_state.save_all();
      network.publish(device_state.topics().t_awake_mode_state.c_str(), "ON", true);
      log_d("[DEVICE] user awake mode set to: ON");
      log_d("[DEVICE] resetting to awake mode...");
    } else if (payload == "OFF") {
      device_state.user_awake_mode = false;
      device_state.save_all();
      network.publish(device_state.topics().t_awake_mode_state.c_str(), "OFF", true);
      log_d("[DEVICE] user awake mode set to: OFF");
    }
    network.publish(device_state.topics().t_awake_mode_cmd.c_str(), nullptr, true);
  }
}

void net_on_connect() {
  network.subscribe(device_state.topics().t_cmd + "#");
  delay(100);
  publish_awake_mode_avlb();
  network.publish(device_state.topics().t_sensor_interval_state.c_str(),
                  String(device_state.sensor_interval()).c_str(), true);
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    auto t = device_state.topics().t_btn_label_state[i];
    network.publish(t.c_str(), device_state.get_btn_label(i), true);
  }
  network.publish(device_state.topics().t_awake_mode_state.c_str(),
                (device_state.user_awake_mode) ? "ON" : "OFF", true);

  if (device_state.send_discovery_config) {
    device_state.send_discovery_config = false;
    send_discovery_config();
  }
}

void button_task(void *param) {
  while (true) {
    for (auto b : buttons) {
      b->update();
    }
    delay(20);
  }
}

void start_button_task() {
  if (button_task_h != nullptr) return;
  log_d("[DEVICE] button task started.");
  xTaskCreate(button_task,    // Function that should be called
              "BUTTON",       // Name of the task (for debugging)
              2000,           // Stack size (bytes)
              NULL,           // Parameter to pass
              23,             // Task priority
              &button_task_h  // Task handle
  );
}

void display_task(void *param) {
  while (true) {
    display.update();
    delay(50);
  }
}

void start_display_task() {
  if (display_task_h != nullptr) return;
  log_d("[DEVICE] display task started.");
  xTaskCreate(display_task,    // Function that should be called
              "DISPLAY",       // Name of the task (for debugging)
              5000,            // Stack size (bytes)
              NULL,            // Parameter to pass
              5,               // Task priority
              &display_task_h  // Task handle
  );
}

void network_task(void *param) {
  while (true) {
    network.update();
    delay(10);
  }
}

void start_network_task() {
  if (network_task_h != nullptr) return;
  log_d("[DEVICE] network task started.");
  xTaskCreate(network_task,    // Function that should be called
              "NETWORK",       // Name of the task (for debugging)
              10000,           // Stack size (bytes)
              NULL,            // Parameter to pass
              10,              // Task priority
              &network_task_h  // Task handle
  );
}

void leds_task(void *param) {
  while (true) {
    leds.update();
    delay(100);
  }
}

void start_leds_task() {
  if (leds_task_h != nullptr) return;
  log_d("[DEVICE] leds task started.");
  xTaskCreate(leds_task,    // Function that should be called
              "LEDS",       // Name of the task (for debugging)
              2000,         // Stack size (bytes)
              NULL,         // Parameter to pass
              23,           // Task priority
              &leds_task_h  // Task handle
  );
}

void main_task(void *param) {
  log_i("[DEVICE] woke up.");
  log_i("[DEVICE] SW version: %s", SW_VERSION);
  device_state.load_all();

  // ------ factory mode ------
  if (device_state.factory().serial_number.length() < 1) {
    log_i("[DEVICE] first boot, starting factory mode...");
    if (factory_mode()) {
      display.begin();
      display.disp_welcome();
      display.update();
      display.end();
      log_i("[DEVICE] factory settings complete. Going to sleep.");
      start_esp_sleep();
    } else {
      // factory_mode() never returns false
    }
  }

  // ------ init hardware ------
  log_i("[HW] version: %s", device_state.factory().hw_version.c_str());
  HW.init(device_state.factory().hw_version);
  display.begin();  // must be before ledAttachPin (reserves GPIO37 = SPIDQS)
  HW.begin();

  // ------ after update handler ------
  if (device_state.last_sw_ver != SW_VERSION) {
    if (device_state.last_sw_ver.length() > 0) {
      log_i("[DEVICE] firmware updated from %s to %s",
            device_state.last_sw_ver.c_str(), SW_VERSION);
      device_state.last_sw_ver = SW_VERSION;
      device_state.send_discovery_config = true;
      device_state.save_all();
      display.disp_message(String("Firmware\nupdated to\n") + SW_VERSION);
      display.update();
      ESP.restart();
    } else {  // first boot after factory flash
      device_state.last_sw_ver = SW_VERSION;
    }
  }

  // ------ determine power mode ------
  device_state.battery_present = HW.is_battery_present();
  device_state.dc_connected = HW.is_dc_connected();
  log_i("[DEVICE] batt present: %d, DC connected: %d",
        device_state.battery_present, device_state.dc_connected);

  if (device_state.battery_present) {
    float batt_voltage = HW.read_battery_voltage();
    log_i("[DEVICE] batt volts: %f", batt_voltage);
    if (device_state.dc_connected) {
      // charging
      if (batt_voltage < HW.CHARGE_HYSTERESIS_VOLT) {
        device_state.charging = true;
        HW.enable_charger(true);
        device_state.awake_mode = true;
      } else {
        device_state.awake_mode = device_state.user_awake_mode;
      }
      device_state.low_batt_mode = false;
    } else {  // dc_connected == false
      if (device_state.low_batt_mode) {
        if (batt_voltage >= HW.BATT_HYSTERESIS_VOLT) {
          device_state.low_batt_mode = false;
          device_state.save_all();
          log_i("[DEVICE] low batt mode disabled");
          ESP.restart();  // to handle display update
        } else {
          log_i("[DEVICE] in low batt mode...");
          go_to_sleep();
        }
      } else {  // low_batt_mode == false
        if (batt_voltage < HW.MIN_BATT_VOLT) {
          // check again
          delay(1000);
          batt_voltage = HW.read_battery_voltage();
          if (batt_voltage < HW.MIN_BATT_VOLT) {
            device_state.low_batt_mode = true;
            log_w("[DEVICE] batt voltage too low, low bat mode enabled");
            display.disp_message_large(
                "Turned\nOFF\n\nPlease\nrecharge\nbattery!");
            display.update();
            go_to_sleep();
          }
        } else if (batt_voltage <= HW.WARN_BATT_VOLT) {
          device_state.battery_low = true;
        }
        device_state.awake_mode = false;
      }
    }
  } else {  // battery_present == false
    if (device_state.dc_connected) {
      device_state.low_batt_mode = false;
      // choose power mode based on user setting
      device_state.awake_mode = device_state.user_awake_mode;
    } else {
      // should never happen
      go_to_sleep();
    }
  }
  log_i("[DEVICE] usr awake mode: %d, awake mode: %d",
        device_state.user_awake_mode, device_state.awake_mode);

  // ------ read sensors ------
  HW.read_temp_hmd(device_state.temperature, device_state.humidity);
  device_state.battery_pct = HW.read_battery_percent();

  // ------ boot cause ------
  BootCause boot_cause;
  int16_t wakeup_pin;
  Button *active_button = nullptr;
  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_EXT1: {
      uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
      wakeup_pin = (log(GPIO_reason)) / log(2);
      log_i("[DEVICE] wakeup cause: PIN %d", wakeup_pin);
      if (wakeup_pin == HW.BTN1_PIN || wakeup_pin == HW.BTN2_PIN ||
          wakeup_pin == HW.BTN3_PIN || wakeup_pin == HW.BTN4_PIN ||
          wakeup_pin == HW.BTN5_PIN || wakeup_pin == HW.BTN6_PIN) {
        boot_cause = BootCause::BUTTON;
      } else {
        boot_cause = BootCause::RESET;
      }
    } break;
    case ESP_SLEEP_WAKEUP_TIMER:
      log_i("[DEVICE] wakeup cause: TIMER");
      boot_cause = BootCause::TIMER;
      break;
    default:
      log_i("[DEVICE] wakeup cause: RESET");
      boot_cause = BootCause::RESET;
      break;
  }

  // ------ handle boot cause ------
  switch (boot_cause) {
    case BootCause::RESET: {
      if (!device_state.silent_restart) {
        display.disp_message("RESTART...", 0);
        display.update();
      }

      if (device_state.restart_to_wifi_setup) {
        device_state.clear_persisted_flags();
        log_i("[DEVICE] staring Wi-Fi setup...");
        start_wifi_setup();  // resets ESP when done
      } else if (device_state.restart_to_setup) {
        device_state.clear_persisted_flags();
        log_i("[DEVICE] staring setup...");
        start_setup();  // resets ESP when done
      }

      device_state.clear_persisted_flags();

      if (!device_state.wifi_done || !device_state.setup_done) {
        display.disp_welcome();
        display.update();
        display.end();
        display.update();
        go_to_sleep();
      } else {
        display.disp_main();
        display.update();
      }
      if (device_state.awake_mode) {
        // proceed with awake mode
        sm_state = StateMachineState::AWAIT_NET_CONNECT;
      } else {
        display.end();
        display.update();
        go_to_sleep();
      }
      break;
    }
    case BootCause::TIMER: {
      if (device_state.awake_mode) {
        // proceed with awake mode
        sm_state = StateMachineState::AWAIT_NET_CONNECT;
      } else {
        if (device_state.info_screen_showing) {
          device_state.info_screen_showing = false;
          display.disp_main();
        }
        if (HW.is_charger_in_standby()) {  // hw <= 2.1 doesn't have awake
                                           // mode when charging
          if (!device_state.charge_complete_showing) {
            device_state.charge_complete_showing = true;
            display.disp_message_large("Fully\ncharged!");
          }
        }
        // proceed with sensor publish
        sm_state = StateMachineState::AWAIT_NET_CONNECT;
      }
      break;
    }
    case BootCause::BUTTON: {
      if (!device_state.wifi_done) {
        start_wifi_setup();
      } else if (!device_state.setup_done) {
        start_setup();
      }

      if (!device_state.awake_mode) {
        if (device_state.info_screen_showing) {
          device_state.info_screen_showing = false;
          display.disp_main();
          display.update();
          display.end();
          display.update();
          go_to_sleep();
        } else if (device_state.charge_complete_showing) {
          device_state.charge_complete_showing = false;
          display.disp_main();
          display.update();
          display.end();
          display.update();
          go_to_sleep();
        } else if (device_state.check_connection) {
          device_state.check_connection = false;
          display.disp_main();
          display.update();
          display.end();
          display.update();
          go_to_sleep();
        } else {
          begin_buttons();
          for (auto b : buttons) {
            if (b->get_pin() == wakeup_pin) {
              active_button = b;
              b->init_press();
            }
          }
          if (active_button != nullptr) {
            start_button_task();
            leds.begin();
            start_leds_task();
            sm_state = StateMachineState::AWAIT_USER_INPUT_FINISH;
          } else {
            go_to_sleep();
          }
        }
      } else {
        // proceed with awake mode
        sm_state = StateMachineState::AWAIT_NET_CONNECT;
      }
      break;
    }
  }

  display.init_ui_state(UIState{.page = DisplayPage::MAIN});
  network.set_callback(mqtt_callback);
  network.set_on_connect(net_on_connect);

  if (!device_state.awake_mode) {
    // ######## SLEEP MODE state machine ########
    esp_task_wdt_init(WDT_TIMEOUT_SLEEP, true);
    esp_task_wdt_add(NULL);
    Button::ButtonAction btn_action = Button::IDLE;
    Button::ButtonAction prev_action = Button::IDLE;
    start_display_task();
    start_network_task();
    network.connect();
    while (true) {
      switch (sm_state) {
        case StateMachineState::AWAIT_USER_INPUT_FINISH: {
          if (active_button == nullptr) {
            log_e("[DEVICE] active button = null");
            sm_state = StateMachineState::CMD_SHUTDOWN;
          } else if (active_button->is_press_finished()) {
            btn_action = active_button->get_action();
            log_d("[DEVICE] BTN_%d pressed - state %s", active_button->get_id(),
                  active_button->get_action_name(btn_action));
            switch (btn_action) {
              case Button::SINGLE:
              case Button::DOUBLE:
              case Button::TRIPLE:
              case Button::QUAD:
                btn_action = btn_action;
                leds.blink(active_button->get_id(),
                           Button::get_action_multi_count(btn_action), true);
                if (device_state.battery_low) {
                  display.disp_message_large(
                      "Battery\nLOW\n\nPlease\nrecharge\nsoon!", 3000);
                }
                sm_state = StateMachineState::AWAIT_NET_CONNECT;
                break;
              case Button::LONG_1:
                // info screen - already displayed by transient action handler
                device_state.info_screen_showing = true;
                log_d("[DEVICE] displayed info screen");
                sm_state = StateMachineState::CMD_SHUTDOWN;
                break;
              case Button::LONG_2:
                device_state.restart_to_setup = true;
                device_state.save_all();
                ESP.restart();
                break;
              case Button::LONG_3:
                device_state.restart_to_wifi_setup = true;
                device_state.save_all();
                ESP.restart();
                break;
              case Button::LONG_4:
                // factory reset
                display.disp_message("Factory\nRESET...");
                network.disconnect(true);  // erase login data
                display.end();
                end_buttons();
                sm_state = StateMachineState::AWAIT_FACTORY_RESET;
                break;
              default:
                sm_state = StateMachineState::CMD_SHUTDOWN;
                break;
            }
            for (auto b : buttons) {
              b->clear();
            }
          } else {
            Button::ButtonAction new_action = active_button->get_action();
            if (new_action != prev_action) {
              switch (new_action) {
                case Button::LONG_1:
                  // info screen
                  display.disp_info();
                  break;
                case Button::LONG_2:
                  display.disp_message(
                      "Release\nfor\nSETUP\n\nKeep\nholding\nfor\nWi-Fi "
                      "SETUP");
                  break;
                case Button::LONG_3:
                  display.disp_message(
                      "Release\nfor\nWi-Fi SETUP\n\nKeep\n"
                      "holding\nfor\nFACTORY\nRESET");
                  break;
                case Button::LONG_4:
                  display.disp_message("Release\nfor\nFACTORY\nRESET");
                  break;
              }
              prev_action = new_action;
            }
          }
          break;
        }
        case StateMachineState::AWAIT_NET_CONNECT: {
          if (network.get_state() == Network::State::M_CONNECTED) {
            // net_on_connect();
            if (active_button != nullptr && btn_action != Button::IDLE) {
              network.publish(
                  device_state
                      .get_button_topic(active_button->get_id(), btn_action)
                      .c_str(),
                  BTN_PRESS_PAYLOAD);
            }
            HW.read_temp_hmd(device_state.temperature, device_state.humidity);
            device_state.battery_pct = HW.read_battery_percent();
            publish_sensors();
            device_state.failed_connections = 0;
            sm_state = StateMachineState::CMD_SHUTDOWN;
          } else if (millis() - network.get_cmd_connect_time() >=
                     NET_CONNECT_TIMEOUT) {
            log_d("[DEVICE] network connect timeout.");
            if (boot_cause == BootCause::BUTTON) {
              display.disp_error("Network\nconnection\nnot\nsuccessful", 3000);
              delay(100);
            } else if (boot_cause == BootCause::TIMER) {
              device_state.failed_connections++;
              if (device_state.failed_connections >= MAX_FAILED_CONNECTIONS) {
                device_state.failed_connections = 0;
                device_state.check_connection = true;
                display.disp_error("Check\nconnection!");
                delay(100);
              }
            }
            sm_state = StateMachineState::CMD_SHUTDOWN;
          }
          break;
        }
        case StateMachineState::CMD_SHUTDOWN: {
          end_buttons();
          leds.end();
          network.disconnect();
          sm_state = StateMachineState::AWAIT_NET_DISCONNECT;
          break;
        }
        case StateMachineState::AWAIT_NET_DISCONNECT: {
          if (network.get_state() == Network::State::DISCONNECTED) {
            if (device_state.display_redraw) {
              device_state.display_redraw = false;
              display.disp_main();
              delay(100);
            }
            display.end();
            sm_state = StateMachineState::AWAIT_SHUTDOWN;
          }
          break;
        }
        case StateMachineState::AWAIT_SHUTDOWN: {
          if (display.get_state() == Display::State::IDLE &&
              leds.get_state() == LEDs::State::IDLE) {
            memory::log_stack_status(button_task_h, display_task_h, network_task_h, leds_task_h, main_task_h);
            if (device_state.awake_mode) {
              device_state.silent_restart = true;
              device_state.save_all();
              ESP.restart();
            } else {
              go_to_sleep();
            }
          }
          break;
        }
        case StateMachineState::AWAIT_FACTORY_RESET: {
          if (network.get_state() == Network::State::DISCONNECTED &&
              display.get_state() == Display::State::IDLE) {
            device_state.clear_all();
            log_i("[DEVICE] factory reset complete.");
            ESP.restart();
          }
          break;
        }
      }  // end switch()
      delay(10);
    }  // end while()
  } else {
    // ######## AWAKE MODE state machine ########
    esp_task_wdt_init(WDT_TIMEOUT_AWAKE, true);
    esp_task_wdt_add(NULL);
    Button::ButtonAction prev_action = Button::IDLE;
    uint32_t last_sensor_publish = 0;
    uint32_t last_display_redraw = 0;
    uint32_t info_screen_start_time = 0;
    device_state.info_screen_showing = false;
    device_state.check_connection = false;
    device_state.charge_complete_showing = false;
    display.disp_main();
    begin_buttons();
    start_button_task();
    leds.begin();
    start_leds_task();
    start_display_task();
    start_network_task();
    network.connect();
    while (true) {
      switch (sm_state) {
        case StateMachineState::AWAIT_NET_CONNECT: {
          if (network.get_state() == Network::State::M_CONNECTED) {
            for (auto b : buttons) {
              b->clear();
            }
            HW.read_temp_hmd(device_state.temperature, device_state.humidity);
            device_state.battery_pct = HW.read_battery_percent();
            publish_sensors();
            sm_state = StateMachineState::AWAIT_USER_INPUT_START;
          } else if (!HW.is_dc_connected()) {
            device_state.charging = false;
            device_state.info_screen_showing = false;
            display.disp_main();
            delay(100);
            sm_state = StateMachineState::CMD_SHUTDOWN;
          }
          break;
        }
        case StateMachineState::AWAIT_USER_INPUT_START: {
          for (auto b : buttons) {
            if (b->get_action() != Button::IDLE) {
              active_button = b;
              break;  // for
            }
          }
          if (active_button != nullptr) {
            sm_state = StateMachineState::AWAIT_USER_INPUT_FINISH;
          } else if (millis() - last_sensor_publish >= AWAKE_SENSOR_INTERVAL) {
            HW.read_temp_hmd(device_state.temperature, device_state.humidity);
            device_state.battery_pct = HW.read_battery_percent();
            publish_sensors();
            last_sensor_publish = millis();
            // log stack status
            memory::log_stack_status(button_task_h, display_task_h, network_task_h, leds_task_h, main_task_h);
          } else if (millis() - last_display_redraw >= AWAKE_REDRAW_INTERVAL) {
            if (device_state.display_redraw) {
              device_state.display_redraw = false;
              if (device_state.info_screen_showing) {
                display.disp_info();
              } else {
                display.disp_main();
              }
            }
            last_display_redraw = millis();
          } else if (device_state.info_screen_showing &&
                     millis() - info_screen_start_time >=
                         INFO_SCREEN_DISP_TIME) {
            display.disp_main();
            device_state.info_screen_showing = false;
          } else if (!HW.is_dc_connected()) {
            publish_awake_mode_avlb();
            device_state.charging = false;
            display.disp_main();
            delay(100);
            sm_state = StateMachineState::CMD_SHUTDOWN;
          }
          if (device_state.charging) {
            if (HW.is_charger_in_standby()) {
              device_state.charging = false;
              display.disp_main();
              if (!device_state.user_awake_mode) {
                delay(100);
                sm_state = StateMachineState::CMD_SHUTDOWN;
              }
            }
          } else {
            if (!device_state.user_awake_mode) {
              display.disp_main();
              delay(100);
              sm_state = StateMachineState::CMD_SHUTDOWN;
            }
          }
          break;
        }
        case StateMachineState::AWAIT_USER_INPUT_FINISH: {
          if (active_button == nullptr) {
            for (auto b : buttons) {
              b->clear();
            }
            sm_state = StateMachineState::AWAIT_USER_INPUT_START;
          } else if (active_button->is_press_finished()) {
            auto btn_action = active_button->get_action();
            log_d("[DEVICE] BTN_%d pressed - state %s", active_button->get_id(),
                  active_button->get_action_name(btn_action));
            switch (btn_action) {
              case Button::SINGLE:
              case Button::DOUBLE:
              case Button::TRIPLE:
              case Button::QUAD:
                if (device_state.info_screen_showing) {
                  display.disp_main();
                  device_state.info_screen_showing = false;
                  sm_state = StateMachineState::AWAIT_USER_INPUT_START;
                  break;
                }
                leds.blink(active_button->get_id(),
                           Button::get_action_multi_count(btn_action));
                network.publish(
                    device_state
                        .get_button_topic(active_button->get_id(), btn_action)
                        .c_str(),
                    BTN_PRESS_PAYLOAD);
                sm_state = StateMachineState::AWAIT_USER_INPUT_START;
                break;
              case Button::LONG_1:
                if (device_state.info_screen_showing) {
                  display.disp_main();
                  device_state.info_screen_showing = false;
                  sm_state = StateMachineState::AWAIT_USER_INPUT_START;
                  break;
                }
                // info screen - already displayed by transient action handler
                device_state.info_screen_showing = true;
                info_screen_start_time = millis();
                log_d("[DEVICE] displayed info screen");
                sm_state = StateMachineState::AWAIT_USER_INPUT_START;
                break;
              case Button::LONG_2:
                device_state.restart_to_setup = true;
                device_state.save_all();
                ESP.restart();
                break;
              case Button::LONG_3:
                device_state.restart_to_wifi_setup = true;
                device_state.save_all();
                ESP.restart();
                break;
              case Button::LONG_4:
                // factory reset
                display.disp_message("Factory\nRESET...");
                network.disconnect(true);  // erase login data
                display.end();
                end_buttons();
                sm_state = StateMachineState::AWAIT_FACTORY_RESET;
                break;
              default:
                sm_state = StateMachineState::AWAIT_USER_INPUT_START;
                break;
            }
            for (auto b : buttons) {
              b->clear();
            }
            active_button = nullptr;
          } else {
            auto new_action = active_button->get_action();
            if (new_action != prev_action) {
              switch (new_action) {
                case Button::LONG_1:
                  // info screen
                  display.disp_info();
                  break;
                case Button::LONG_2:
                  display.disp_message(
                      "Release\nfor\nSETUP\n\nKeep\nholding\nfor\nWi-Fi "
                      "SETUP");
                  break;
                case Button::LONG_3:
                  display.disp_message(
                      "Release\nfor\nWi-Fi SETUP\n\nKeep\n"
                      "holding\nfor\nFACTORY\nRESET");
                  break;
                case Button::LONG_4:
                  display.disp_message("Release\nfor\nFACTORY\nRESET");
                  break;
              }
              prev_action = new_action;
            }
          }
          break;
        }
        case StateMachineState::CMD_SHUTDOWN: {
          device_state.info_screen_showing = false;
          end_buttons();
          leds.end();
          network.disconnect();
          sm_state = StateMachineState::AWAIT_NET_DISCONNECT;
          break;
        }
        case StateMachineState::AWAIT_NET_DISCONNECT: {
          if (network.get_state() == Network::State::DISCONNECTED) {
            if (device_state.display_redraw) {
              device_state.display_redraw = false;
              display.disp_main();
              delay(100);
            }
            display.end();
            sm_state = StateMachineState::AWAIT_SHUTDOWN;
          }
          break;
        }
        case StateMachineState::AWAIT_SHUTDOWN: {
          if (display.get_state() == Display::State::IDLE &&
              leds.get_state() == LEDs::State::IDLE) {
            go_to_sleep();
          }
          break;
        }
        case StateMachineState::AWAIT_FACTORY_RESET: {
          if (network.get_state() == Network::State::DISCONNECTED &&
              display.get_state() == Display::State::IDLE) {
            device_state.clear_all();
            log_i("[DEVICE] factory reset complete.");
            ESP.restart();
          }
          break;
        }
      }  // end switch()
      if (ESP.getMinFreeHeap() < MIN_FREE_HEAP) {
        log_w("[DEVICE] free heap low, restarting...");
        memory::log_stack_status(button_task_h, display_task_h, network_task_h, leds_task_h, main_task_h);
        device_state.silent_restart = true;
        device_state.save_all();
        ESP.restart();
      }
      esp_task_wdt_reset();
      delay(10);
    }  // end while()
  }    // end else
}

void setup() {
  xTaskCreate(main_task,    // Function that should be called
              "MAIN",       // Name of the task (for debugging)
              20000,        // Stack size (bytes)
              NULL,         // Parameter to pass
              1,            // Task priority
              &main_task_h  // Task handle
  );
  log_d("[DEVICE] main task started.");

  vTaskDelete(NULL);

}  // end setup()

void loop() {}

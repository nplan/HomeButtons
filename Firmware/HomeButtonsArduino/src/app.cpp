#include "app.h"

#include <Arduino.h>
#include <esp_task_wdt.h>
#include <SPIFFS.h>

#include "config.h"
#include "factory.h"
#include "setup.h"
#include "hardware.h"

#include "mdi_helper.h"

static constexpr uint32_t MDI_FREE_SPACE_THRESHOLD = 100000UL;

App::App()
    : AppStateMachine("AppSM", *this),
      Logger("APP"),
      network_(device_state_),
      display_(device_state_, mdi_),
      mqtt_(device_state_, network_) {}

void App::setup() {
  info("starting...");
  xTaskCreate(_main_task_helper,  // Function that should be called
              "MAIN",             // Name of the task (for debugging)
              10000,              // Stack size (bytes)
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
    } else if (device_state_.flags().schedule_wakeup_time > 0) {
      esp_sleep_enable_timer_wakeup(device_state_.flags().schedule_wakeup_time *
                                    1000000UL);
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

std::pair<BootCause, int16_t> App::_determine_boot_cause() {
  BootCause boot_cause = BootCause::RESET;
  int16_t wakeup_pin = 0;
  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_EXT1: {
      uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
      wakeup_pin = (log(GPIO_reason)) / log(2);
      info("wakeup cause: PIN %d", wakeup_pin);
      if (button_handler_.is_button(wakeup_pin)) {
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
  return std::make_pair(boot_cause, wakeup_pin);
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

void App::_button_task(void* param) {
  App* app = static_cast<App*>(param);
  while (true) {
    app->button_handler_.update();
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
  xTaskCreate(
      _button_task,  // Function that should be called
      "BUTTON",      // Name of the task (for debugging)
      2000,          // Stack size (bytes)
      this,          // Parameter to pass
      23,            // Task priority, using same as wifi driver:
           // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/performance/speed.html
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
              15000,            // Stack size (bytes)
              this,             // Parameter to pass
              1,                // Task priority
              &network_task_h_  // Task handle
  );
}

void App::_start_leds_task() {
  if (leds_task_h_ != nullptr) return;
  debug("leds task started.");
  xTaskCreate(
      &_leds_task,  // Function that should be called
      "LEDS",       // Name of the task (for debugging)
      2000,         // Stack size (bytes)
      this,         // Parameter to pass
      23,           // Task priority, using same as wifi driver:
           // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/performance/speed.html
      &leds_task_h_  // Task handle
  );
}

void App::_main_task() {
  info("woke up.");
  info("cpu freq: %d MHz", getCpuFrequencyMhz());
  info("SW version: %s", SW_VERSION);

  // ------ init hardware ------
  if (!hw_.init()) {
    error("HW init failed! Going to sleep.");
    _start_esp_sleep();
  }

  // ------ factory test ------
  FactoryTest factory_test;
  if (factory_test.is_test_required()) {
    if (factory_test.run_test(hw_, display_)) {
      device_state_.load_all(hw_);
      display_.begin(hw_);
      display_.disp_welcome();
      display_.update();
      display_.end();
      info("factory test complete. Going to sleep.");
      _start_esp_sleep();
    } else {
      error("factory test failed! Going to sleep.");
      display_.disp_error("Factory\nTest\nFailed");
      display_.update();
      _start_esp_sleep();
    }
  }

  device_state_.load_all(hw_);

  // must be before ledAttachPin (reserves GPIO37 = SPIDQS)
  display_.begin(hw_);
  hw_.begin();
  leds_.begin();
#ifndef HOME_BUTTONS_MINI
  std::tuple<uint8_t, uint16_t, boolean> button_map[NUM_BUTTONS] = {
      {hw_.BTN1_PIN, 1, true}, {hw_.BTN6_PIN, 2, true},
      {hw_.BTN2_PIN, 3, true}, {hw_.BTN5_PIN, 4, true},
      {hw_.BTN3_PIN, 5, true}, {hw_.BTN4_PIN, 6, true}};
#else
  std::tuple<uint8_t, uint16_t, boolean> button_map[NUM_BUTTONS] = {
      {hw_.BTN1_PIN, 1, true},
      {hw_.BTN2_PIN, 2, true},
      {hw_.BTN3_PIN, 3, true},
      {hw_.BTN4_PIN, 4, true}};
#endif
  button_handler_.begin(button_map);

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
#ifndef HOME_BUTTONS_MINI
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
          ESP.restart();  // to handle display update
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
#else
  float batt_voltage = hw_.read_battery_voltage();
  info("batt volts: %f", batt_voltage);

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
            "Turned\nOFF\n\nPlease\nreplace\nbatteries!");
        display_.update();
        _go_to_sleep();
      }
    } else if (batt_voltage <= hw_.WARN_BATT_VOLT) {
      device_state_.sensors().battery_low = true;
    }
  }
  // mini doesn't have awake mode
  device_state_.flags().awake_mode = false;

#endif

  // ------ read sensors ------
  hw_.read_temp_hmd(device_state_.sensors().temperature,
                    device_state_.sensors().humidity,
                    device_state_.get_use_fahrenheit());
  device_state_.sensors().battery_pct = hw_.read_battery_percent();
  device_state_.sensors().battery_voltage = hw_.read_battery_voltage();

  // ------ boot cause ------
  int16_t wakeup_pin;
  std::tie(boot_cause_, wakeup_pin) = _determine_boot_cause();

  // ------ handle boot cause ------
  switch (boot_cause_) {
    case BootCause::RESET: {
      if (!device_state_.persisted().silent_restart) {
        hw_.set_all_leds(LED_DFLT_BRIGHT);
        display_.disp_message("RESTART...", 0);
        display_.update();
      }

      // format SPIFFS if needed
      if (!SPIFFS.begin()) {
        info("Formatting icon storage...");
        display_.disp_message("Formatting\nIcon\nStorage...", 0);
        display_.update();
        SPIFFS.format();
      } else {
        SPIFFS.end();
        debug("SPIFFS test mount OK");
      }

      if (device_state_.persisted().restart_to_wifi_setup) {
        device_state_.clear_persisted_flags();
        info("staring Wi-Fi setup...");
        start_wifi_setup(device_state_, display_);  // resets ESP when done
      } else if (device_state_.persisted().restart_to_setup) {
        device_state_.clear_persisted_flags();
        info("staring setup...");
        start_setup(device_state_, display_, hw_);  // resets ESP when done
      } else if (device_state_.persisted().connect_on_restart) {
        device_state_.clear_persisted_flags();
        device_state_.persisted().download_mdi_icons = true;
        device_state_.persisted().send_discovery_config = true;
        device_state_.save_all();
        display_.disp_message("Updating\nconfiguration\n...");
        display_.update();
        debug("connecting on restart...");
        break;
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
      device_state_.persisted().download_mdi_icons = true;
      device_state_.persisted().send_discovery_config = true;
      device_state_.save_all();
      if (device_state_.flags().awake_mode) {
        // proceed with awake mode
        hw_.set_all_leds(0);
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
      }
      break;
    }
    case BootCause::BUTTON: {
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
        } else if (device_state_.persisted().user_msg_showing) {
          device_state_.persisted().user_msg_showing = false;
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
          button_handler_.init_press(wakeup_pin);
        }
      } else {
        // proceed with awake mode
      }
      break;
    }
  }

  display_.init_ui_state(UIState{.page = DisplayPage::MAIN});
  network_.set_mqtt_callback(std::bind(&App::_mqtt_callback, this,
                                       std::placeholders::_1,
                                       std::placeholders::_2));
  network_.set_on_connect(std::bind(&App::_net_on_connect, this));

  debug("Starting main loop");
  while (true) {
    loop();
    esp_task_wdt_reset();
    delay(10);
  }
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
      info("Updating discovery config...");
      mqtt_.update_discovery_config();
      debug("sensor interval set to %d minutes", mins);
      _publish_sensors();
    }
    network_.publish(mqtt_.t_sensor_interval_cmd(), "", true);
    return;
  }

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    if (strcmp(topic, mqtt_.t_btn_label_cmd(i).c_str()) == 0) {
      ButtonLabel new_label(payload);
      new_label = new_label.trim();
      debug("button %d label changed to: %s", i + 1, new_label.c_str());
      device_state_.set_btn_label(i, new_label.c_str());

      network_.publish(mqtt_.t_btn_label_state(i),
                       device_state_.get_btn_label(i), true);
      network_.publish(mqtt_.t_btn_label_cmd(i), "", true);
      device_state_.flags().display_redraw = true;
      device_state_.save_all();

      ButtonLabel label(device_state_.get_btn_label(i).c_str());

      if (label.substring(0, 4) == "mdi:") {
        device_state_.persisted().download_mdi_icons = true;
      }
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

  // user message
  if (strcmp(topic, mqtt_.t_disp_msg_cmd().c_str()) == 0) {
    if (display_.get_ui_state().page == DisplayPage::MAIN) {
      UserMessage msg(payload);
      device_state_.persisted().user_msg_showing = true;
      device_state_.save_all();
      display_.disp_message_large(msg.c_str());
    }
    network_.publish(mqtt_.t_disp_msg_cmd(), "", true);
    network_.publish(mqtt_.t_disp_msg_state(), "-", false);
  }

  // schedule wakeup cmd
  if (strcmp(topic, mqtt_.t_schedule_wakeup_cmd().c_str()) == 0) {
    uint32_t secs = atoi(payload);
    if (secs >= SCHEDULE_WAKEUP_MIN && secs <= SCHEDULE_WAKEUP_MAX) {
      device_state_.flags().schedule_wakeup_time = secs;
      network_.publish(mqtt_.t_schedule_wakeup_cmd(), "", true);
      network_.publish(mqtt_.t_schedule_wakeup_state(), "None", true);
      debug("schedule wakeup set to %d seconds", secs);
    }
  }
}

void App::_net_on_connect() {
  network_.subscribe(mqtt_.t_cmd() + "#");
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
  network_.publish(mqtt_.t_disp_msg_state(), "-", false);

  if (device_state_.persisted().send_discovery_config) {
    device_state_.persisted().send_discovery_config = false;
    info("Sending discovery config...");
    mqtt_.send_discovery_config();
  }
}

void App::_download_mdi_icons() {
  bool download_required = false;
  mdi_.begin();
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    ButtonLabel label(device_state_.get_btn_label(i).c_str());
    if (label.substring(0, 4) == "mdi:") {
      MDIName icon = label.substring(
          4, label.index_of(' ') > 0 ? label.index_of(' ') : label.length());
      if (!mdi_.exists_all_sizes(icon.c_str())) {
        download_required = true;
        break;
      }
    }
  }
  if (!download_required) {
    info("no icons to download");
    mdi_.end();
    return;
  }

  display_.disp_message("Downloading\nicons...");

  // check if server is reachable
  if (mdi_.check_connection()) {
    info("icon server reachable");
  } else {
    warning("icon server NOT reachable");
    mdi_.end();
    display_.disp_error("Icon\nserver\nNOT\nreachable");
    device_state_.flags().display_redraw = true;
    return;
  }

  // free up space if needed
  size_t free = mdi_.get_free_space();
  info("SPIFFS free space: %d", free);
  if (free < MDI_FREE_SPACE_THRESHOLD) {
    info("making space...");
    if (!mdi_.make_space(2 * MDI_FREE_SPACE_THRESHOLD)) {
      error("failed to make space");
      mdi_.end();
      return;
    }
  }

  info("Downloading icons...");
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    ButtonLabel label(device_state_.get_btn_label(i).c_str());
    if (label.substring(0, 4) == "mdi:") {
      MDIName icon = label.substring(
          4, label.index_of(' ') > 0 ? label.index_of(' ') : label.length());
      if (!mdi_.exists_all_sizes(icon.c_str())) {
        mdi_.download(icon.c_str());
      }
    }
  }
  mdi_.end();
  device_state_.flags().display_redraw = true;
}

void AppSMStates::InitState::entry() {
  sm().network_.connect();

  sm()._start_button_task();
  sm()._start_leds_task();
  sm()._start_display_task();
  sm()._start_network_task();

#ifndef HOME_BUTTONS_MINI
  sm().mdi_.add_size(64);
  sm().mdi_.add_size(48);
#else
  sm().mdi_.add_size(100);
#endif

  if (!sm().device_state_.flags().awake_mode) {
    esp_task_wdt_init(WDT_TIMEOUT_SLEEP, true);
    esp_task_wdt_add(NULL);
    if (sm().boot_cause_ == BootCause::TIMER ||
        sm().boot_cause_ == BootCause::RESET) {
      return transition_to<NetConnectingState>();
    } else {  // button
      return transition_to<UserInputFinishState>();
    }
  } else {
    sm().device_state_.persisted().info_screen_showing = false;
    sm().device_state_.persisted().check_connection = false;
    sm().device_state_.persisted().charge_complete_showing = false;
    esp_task_wdt_init(WDT_TIMEOUT_AWAKE, true);
    esp_task_wdt_add(NULL);
    sm().display_.disp_main();
    return transition_to<AwakeModeIdleState>();
  }
}

void AppSMStates::AwakeModeIdleState::entry() { sm().button_handler_.clear(); }

void AppSMStates::AwakeModeIdleState::loop() {
  if (sm().button_handler_.is_press_in_progress()) {
    return transition_to<UserInputFinishState>();
  } else if (millis() - sm().last_sensor_publish_ >= AWAKE_SENSOR_INTERVAL) {
    sm().hw_.read_temp_hmd(sm().device_state_.sensors().temperature,
                           sm().device_state_.sensors().humidity,
                           sm().device_state_.get_use_fahrenheit());
    sm().device_state_.sensors().battery_pct = sm().hw_.read_battery_percent();
    sm().device_state_.sensors().battery_voltage =
        sm().hw_.read_battery_voltage();
    sm()._publish_sensors();
    sm().last_sensor_publish_ = millis();
    sm()._log_stack_status();
  } else if (millis() - sm().last_m_display_redraw_ >= AWAKE_REDRAW_INTERVAL) {
    if (sm().device_state_.flags().display_redraw) {
      sm().device_state_.flags().display_redraw = false;
      if (sm().device_state_.persisted().download_mdi_icons) {
        sm()._download_mdi_icons();
        sm().device_state_.persisted().download_mdi_icons = false;
      }
      if (sm().device_state_.persisted().info_screen_showing) {
        sm().display_.disp_info();
      } else {
        sm().display_.disp_main();
      }
    }
    sm().last_m_display_redraw_ = millis();
  } else if (sm().device_state_.persisted().info_screen_showing &&
             millis() - sm().info_screen_start_time_ >= INFO_SCREEN_DISP_TIME) {
    sm().display_.disp_main();
    sm().device_state_.persisted().info_screen_showing = false;
  } else if (!sm().hw_.is_dc_connected()) {
    sm()._publish_awake_mode_avlb();
    sm().device_state_.sensors().charging = false;
    return transition_to<CmdShutdownState>();
  }
  if (sm().device_state_.sensors().charging) {
    if (sm().hw_.is_charger_in_standby()) {
      sm().device_state_.sensors().charging = false;
      sm().display_.disp_main();
      if (!sm().device_state_.persisted().user_awake_mode) {
        return transition_to<CmdShutdownState>();
      }
    }
  } else {
    if (!sm().device_state_.persisted().user_awake_mode) {
      return transition_to<CmdShutdownState>();
    }
  }
}

void AppSMStates::UserInputFinishState::loop() {
  bool awake_mode = sm().device_state_.flags().awake_mode;

  ButtonEvent btn_event = sm().button_handler_.get_event();
  if (sm().button_handler_.is_press_finished()) {
    sm().debug("BTN_%d pressed - state %s", btn_event.id,
               Button::get_action_name(btn_event.action));

    switch (btn_event.action) {
      case Button::SINGLE:
      case Button::DOUBLE:
      case Button::TRIPLE:
      case Button::QUAD:
        if (!sm().device_state_.persisted().wifi_done) {
          sm().device_state_.persisted().restart_to_wifi_setup = true;
          sm().device_state_.persisted().silent_restart = true;
          sm().device_state_.save_all();
          ESP.restart();
        } else if (!sm().device_state_.persisted().setup_done) {
          sm().device_state_.persisted().restart_to_setup = true;
          sm().device_state_.persisted().silent_restart = true;
          sm().device_state_.save_all();
          ESP.restart();
        }
        if (awake_mode) {
          if (sm().device_state_.persisted().info_screen_showing) {
            sm().display_.disp_main();
            sm().device_state_.persisted().info_screen_showing = false;
            return transition_to<AwakeModeIdleState>();
          } else if (sm().device_state_.persisted().user_msg_showing) {
            sm().display_.disp_main();
            sm().device_state_.persisted().user_msg_showing = false;
            return transition_to<AwakeModeIdleState>();
          }
          sm().leds_.blink(btn_event.id,
                           Button::get_action_multi_count(btn_event.action));
          sm().network_.publish(sm().mqtt_.get_button_topic(btn_event),
                                BTN_PRESS_PAYLOAD);
          return transition_to<AwakeModeIdleState>();
        } else {
          sm().leds_.blink(btn_event.id,
                           Button::get_action_multi_count(btn_event.action),
                           true);
          if (sm().device_state_.sensors().battery_low) {
#ifndef HOME_BUTTONS_MINI
            sm().display_.disp_message_large(
                "Battery\nLOW\n\nPlease\nrecharge\nsoon!", 3000);
#else
            sm().display_.disp_message_large(
                "Batteries\nLOW\n\nPlease\nreplace\nsoon!", 3000);
#endif
          }
          sm().btn_event_ = btn_event;
          sm().button_handler_.clear();
          return transition_to<NetConnectingState>();
        }
        break;
      default:
        if (awake_mode) {
          return transition_to<AwakeModeIdleState>();
        } else {
          return transition_to<CmdShutdownState>();
        }
        break;
    }
  } else {  // button press not finished
    if (btn_event.action != sm().btn_event_.action) {
      switch (btn_event.action) {
        case Button::LONG_1:
          // info screen
          if (sm().hw_.num_buttons_pressed() == 1) {
            sm().device_state_.persisted().info_screen_showing = true;
            sm().info_screen_start_time_ = millis();
            sm().display_.disp_info();
            sm().debug("displayed info screen");
            if (awake_mode) {
              return transition_to<AwakeModeIdleState>();
            } else {
              return transition_to<CmdShutdownState>();
            }
          }
          break;
        case Button::LONG_2:
          if (sm().hw_.num_buttons_pressed() == 2) {
            return transition_to<SettingsMenuState>();
          }
          break;
        default:
          break;
      }
      sm().btn_event_ = btn_event;
    }
  }
}

void AppSMStates::NetConnectingState::loop() {
  if (sm().network_.get_state() == Network::State::M_CONNECTED) {
    if (sm().btn_event_.action != Button::IDLE) {
      sm().network_.publish(sm().mqtt_.get_button_topic(sm().btn_event_),
                            BTN_PRESS_PAYLOAD);
    }
    sm().hw_.read_temp_hmd(sm().device_state_.sensors().temperature,
                           sm().device_state_.sensors().humidity,
                           sm().device_state_.get_use_fahrenheit());
    sm().device_state_.sensors().battery_pct = sm().hw_.read_battery_percent();
    sm()._publish_sensors();
    sm().device_state_.persisted().failed_connections = 0;
    return transition_to<CmdShutdownState>();
  } else if (sm().button_handler_.is_press_finished()) {
    // abort on button press
    if (sm().button_handler_.get_event().action == Button::SINGLE) {
      sm().info("button press - user cancelled, aborting...");
      return transition_to<CmdShutdownState>();
    } else {
      sm().button_handler_.clear();
    }

  } else if (millis() >= NET_CONNECT_TIMEOUT) {
    sm().warning("network connect timeout.");
    if (sm().boot_cause_ == BootCause::BUTTON) {
      sm().display_.disp_error("Network\nconnection\nnot\nsuccessful", 3000);
    } else if (sm().boot_cause_ == BootCause::TIMER) {
      sm().device_state_.persisted().failed_connections++;
      if (sm().device_state_.persisted().failed_connections >=
          MAX_FAILED_CONNECTIONS) {
        sm().device_state_.persisted().failed_connections = 0;
        sm().device_state_.persisted().check_connection = true;
        sm().display_.disp_error("Check\nconnection!");
      }
    }
    return transition_to<CmdShutdownState>();
  }
}

void AppSMStates::SettingsMenuState::entry() {
  sm().settings_menu_start_time_ = millis();
  sm().device_state_.persisted().info_screen_showing = false;
  sm().display_.disp_settings();
  // wait until button released. TODO: should be improved
  while (sm().hw_.any_button_pressed()) {
    delay(10);
  }
  sm().button_handler_.clear();
}

void AppSMStates::SettingsMenuState::loop() {
  bool awake_mode = sm().device_state_.flags().awake_mode;
  ButtonEvent btn_event = sm().button_handler_.get_event();
  if (sm().button_handler_.is_press_in_progress()) {
    if (sm().button_handler_.is_press_finished()) {
      if (btn_event.action == Button::SINGLE) {
        switch (btn_event.id) {
          case 1:
            // setup
            sm().device_state_.persisted().restart_to_setup = true;
            sm().device_state_.persisted().silent_restart = true;
            sm().device_state_.save_all();
            ESP.restart();
            break;
          case 2:
            // Wi-Fi setup
            sm().device_state_.persisted().restart_to_wifi_setup = true;
            sm().device_state_.persisted().silent_restart = true;
            sm().device_state_.save_all();
            ESP.restart();
            break;
          case 3:
            // restart
            ESP.restart();
            break;
          case 4:
            // cancel
            sm().display_.disp_main();
            if (awake_mode) {
              return transition_to<AwakeModeIdleState>();
            } else {
              return transition_to<CmdShutdownState>();
            }
            break;
          default:
            break;
        }
      }
      sm().button_handler_.clear();
    } else if (btn_event.action == Button::LONG_3 && btn_event.id == 3) {
      // factory reset
      return transition_to<FactoryResetState>();
    } else if (btn_event.action == Button::LONG_1 && btn_event.id == 1) {
      // device info screen
      return transition_to<DeviceInfoState>();
    }
  }
  if (!awake_mode &&
      millis() - sm().settings_menu_start_time_ > SETTINGS_MENU_TIMEOUT &&
      !sm().button_handler_.is_press_in_progress()) {
    sm().debug("settings menu timeout");
    sm().display_.disp_main();
    return transition_to<CmdShutdownState>();
  }
}

void AppSMStates::DeviceInfoState::entry() {
  sm().device_info_start_time_ = millis();
  sm().display_.disp_device_info();
  // wait until button released. TODO: should be improved
  while (sm().hw_.any_button_pressed()) {
    delay(10);
  }
  sm().button_handler_.clear();
}

void AppSMStates::DeviceInfoState::loop() {
  bool awake_mode = sm().device_state_.flags().awake_mode;
  ButtonEvent btn_event = sm().button_handler_.get_event();
  if (sm().button_handler_.is_press_in_progress()) {
    if (sm().button_handler_.is_press_finished()) {
      if (btn_event.action == Button::SINGLE) {
        // cancel
        sm().display_.disp_main();
        sm().button_handler_.clear();
        if (awake_mode) {
          return transition_to<AwakeModeIdleState>();
        } else {
          return transition_to<CmdShutdownState>();
        }
      }
      sm().button_handler_.clear();
    }
  }
  if (!awake_mode &&
      millis() - sm().device_info_start_time_ > DEVICE_INFO_TIMEOUT) {
    sm().debug("device info timeout");
    sm().display_.disp_main();
    return transition_to<CmdShutdownState>();
  }
}

void AppSMStates::CmdShutdownState::entry() {
  if (sm().device_state_.persisted().download_mdi_icons) {
    sm()._download_mdi_icons();
    sm().device_state_.persisted().download_mdi_icons = false;
  }
  if (sm().boot_cause_ == BootCause::RESET) {
    sm().display_.disp_main();
  }
  if (sm().device_state_.flags().awake_mode) {
    sm().device_state_.persisted().info_screen_showing = false;
    sm().display_.disp_main();
  }
  sm().shutdown_cmd_time_ = millis();
}

void AppSMStates::CmdShutdownState::loop() {
  // wait for timeout
  if (millis() - sm().shutdown_cmd_time_ > SHUTDOWN_DELAY) {
    sm().button_handler_.end();
    sm().leds_.end();
    sm().network_.disconnect();
    return transition_to<NetDisconnectingState>();
  }
}

void AppSMStates::NetDisconnectingState::loop() {
  if (sm().network_.get_state() == Network::State::DISCONNECTED &&
      !sm().display_.busy()) {
    if (sm().device_state_.flags().display_redraw) {
      sm().device_state_.flags().display_redraw = false;
      sm().display_.disp_main();
    }
    sm().display_.end();
    return transition_to<ShuttingDownState>();
  }
}

void AppSMStates::ShuttingDownState::loop() {
  if (sm().display_.get_state() == Display::State::IDLE &&
      sm().leds_.get_state() == LEDs::State::IDLE &&
      !sm().hw_.any_button_pressed()) {
    sm()._log_stack_status();
    if (sm().device_state_.flags().awake_mode) {
      sm().device_state_.persisted().silent_restart = true;
      sm().device_state_.save_all();
      ESP.restart();
    } else {
      sm()._go_to_sleep();
    }
  }
}

void AppSMStates::FactoryResetState::entry() {
  sm().display_.disp_message("Factory\nRESET...");
  sm().network_.disconnect(true);  // erase login data
  sm().display_.end();
  sm().button_handler_.end();
}

void AppSMStates::FactoryResetState::loop() {
  if (sm().network_.get_state() == Network::State::DISCONNECTED &&
      sm().display_.get_state() == Display::State::IDLE) {
    sm().device_state_.clear_all();
    sm().info("factory reset complete.");
    ESP.restart();
  }
}

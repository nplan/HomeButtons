#include "app.h"

#include <Arduino.h>
#include <esp_task_wdt.h>
#include <SPIFFS.h>
#include "esp_ota_ops.h"

#include "config.h"
#include "factory.h"
#include "setup.h"
#include "hardware.h"

#include "mdi_helper.h"

extern "C" bool verifyRollbackLater() { return true; };

App::App()
    : AppStateMachine("AppSM", *this),
      Logger("APP"),
#if defined(HOME_BUTTONS_ORIGINAL)
      btn1_("BTN1", 1, hw_),
      btn2_("BTN2", 2, hw_),
      btn3_("BTN3", 3, hw_),
      btn4_("BTN4", 4, hw_),
      btn5_("BTN5", 5, hw_),
      btn6_("BTN6", 6, hw_),
      buttons_("BtnHdlr",
               std::array<std::reference_wrapper<Button>, NUM_BUTTONS>{
                   btn1_, btn2_, btn3_, btn4_, btn5_, btn6_}),
      led1_("LED1", 1, hw_),
      led2_("LED2", 2, hw_),
      led3_("LED3", 3, hw_),
      led4_("LED4", 4, hw_),
      led5_("LED5", 5, hw_),
      led6_("LED6", 6, hw_),
      leds_("LEDs",
            std::array<std::reference_wrapper<LED>, NUM_BUTTONS>{
                led1_, led2_, led3_, led4_, led5_, led6_}),
#elif defined(HOME_BUTTONS_MINI)
      btn1_("BTN1", 1, hw_),
      btn2_("BTN2", 2, hw_),
      btn3_("BTN3", 3, hw_),
      btn4_("BTN4", 4, hw_),
      buttons_("BtnHdlr",
               std::array<std::reference_wrapper<Button>, NUM_BUTTONS>{
                   btn1_, btn2_, btn3_, btn4_}),
      led1_("LED1", 1, hw_),
      led2_("LED2", 2, hw_),
      led3_("LED3", 3, hw_),
      led4_("LED4", 4, hw_),

      leds_("LEDs",
            std::array<std::reference_wrapper<LED>, NUM_BUTTONS>{led1_, led2_,
                                                                 led3_, led4_}),
#elif defined(HOME_BUTTONS_PRO)
      touch_handler_(hw_),
#endif
      network_(device_state_),
      display_(device_state_, mdi_),
      mqtt_(device_state_, network_) {
}

void App::setup() {
  info("starting...");
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
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
  esp_sleep_enable_ext1_wakeup(hw_.WAKE_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
  if (device_state_.persisted().wifi_done &&
      device_state_.persisted().setup_done &&
      !device_state_.persisted().low_batt_mode &&
      !device_state_.persisted().check_connection) {
    if (device_state_.flags().schedule_wakeup_time > 0) {
      esp_sleep_enable_timer_wakeup(device_state_.flags().schedule_wakeup_time *
                                    1000000UL);
    } else {
      esp_sleep_enable_timer_wakeup(device_state_.sensor_interval() *
                                    60000000UL);
    }
  }
#elif defined(HOME_BUTTONS_PRO)
  esp_sleep_enable_ext1_wakeup(hw_.WAKE_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
#endif
  info("deep sleep... z z z");
  esp_deep_sleep_start();
}

void App::_go_to_sleep() {
  device_state_.save_all();
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
  hw_.set_all_leds(0);
#elif defined(HOME_BUTTONS_PRO)
  hw_.set_frontlight(0);
#endif
  _start_esp_sleep();
}

std::pair<BootCause, int16_t> App::_determine_boot_cause() {
  BootCause boot_cause = BootCause::RESET;
  int16_t wakeup_pin = 0;
  uint8_t wakeup_btn_id = 0;
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_EXT1: {
      uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
      wakeup_pin = (log(GPIO_reason)) / log(2);
      info("wakeup cause: PIN %d", wakeup_pin);
      wakeup_btn_id = buttons_.id_from_pin(wakeup_pin);
      info("wakeup btn id: %d", wakeup_btn_id);
      if (wakeup_btn_id > 0) {
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
#elif defined(HOME_BUTTONS_PRO)
  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_EXT1:
      info("wakeup cause: BUTTON");
      boot_cause = BootCause::BUTTON;
      break;
    default:
      info("wakeup cause: RESET");
      boot_cause = BootCause::RESET;
      break;
  }
#endif
  return std::make_pair(boot_cause, wakeup_btn_id);
}

void App::_log_task_stats() {
  const UBaseType_t maxTasks = 20;
  TaskStatus_t statusArray[maxTasks];
  uint32_t totalRunTime;

  // Fetch the status of tasks
  UBaseType_t numTasks =
      uxTaskGetSystemState(statusArray, maxTasks, &totalRunTime);

  // Print task stats
  debug("#### Task Stats ####");
  debug("%-15s%10s%10s%10s%10s%10s", "Task Name", "State", "Prio", "Stack",
        "Num", "Time %");
  for (UBaseType_t i = 0; i < numTasks; i++) {
    TaskStatus_t* taskStatus = &statusArray[i];

    // Calculate task run time as a percentage
    float runTimePercentage = 0.0;
    if (totalRunTime > 0) {
      runTimePercentage =
          (taskStatus->ulRunTimeCounter / (float)totalRunTime) * 100;
    }

    char buffer[128];
    sprintf(buffer, "%-15s%10d%10d%10d%10d%10.2f", taskStatus->pcTaskName,
            taskStatus->eCurrentState, taskStatus->uxCurrentPriority,
            taskStatus->usStackHighWaterMark, (int)taskStatus->xTaskNumber,
            runTimePercentage);
    debug(buffer);
  }
  uint32_t esp_free_heap = ESP.getFreeHeap();
  uint32_t esp_min_free_heap = ESP.getMinFreeHeap();
  uint32_t rtos_free_heap = xPortGetFreeHeapSize();
  debug("Free heap: ESP %d, ESP MIN %d, RTOS %d\n", esp_free_heap,
        esp_min_free_heap, rtos_free_heap);
}

void App::_ui_task(void* param) {
  App* app = static_cast<App*>(param);
  while (true) {
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
    app->buttons_.Loop();
    app->leds_.Loop();
#elif defined(HOME_BUTTONS_PRO)
    app->touch_handler_.Loop();
    if (millis() - app->device_state_.flags().last_user_input_time >
        FRONTLIGHT_TIMEOUT) {
      if (!app->device_state_.flags().keep_frontlight_on) {
        app->hw_.set_frontlight(0);
      }
    }
#endif
    delay(10);
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

void App::_start_ui_task() {
  if (ui_task_h_ != nullptr) return;
  debug("UI task started.");
  xTaskCreate(
      _ui_task,  // Function that should be called
      "UI",      // Name of the task (for debugging)
      10000,     // Stack size (bytes)
      this,      // Parameter to pass
      23,        // Task priority, using same as wifi driver:
           // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/performance/speed.html
      &ui_task_h_  // Task handle
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

void App::_main_task() {
  info("woke up.");
  info("cpu freq: %d MHz", getCpuFrequencyMhz());
  info("SW version: %s", SW_VERSION);

  // ------ init hardware ------
  bool hw_init_ok = hw_.init();

  // verify OTA if this is first boot after OTA
  const esp_partition_t* running = esp_ota_get_running_partition();
  esp_ota_img_states_t ota_state;
  if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
      if (hw_init_ok) {
        esp_ota_mark_app_valid_cancel_rollback();
      } else {
        error("OTA verification failed! Rolling back...");
        esp_ota_mark_app_invalid_rollback_and_reboot();
        ESP.restart();  // if above line fails
      }
    }
  }

  if (!hw_init_ok) {
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
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
  buttons_.Init();
  leds_.Init();
#elif defined(HOME_BUTTONS_PRO)
  touch_handler_.Init(hw_.TOUCH_CLICK_PIN, hw_.TOUCH_INT_PIN);
#endif

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
#if defined(HOME_BUTTONS_ORIGINAL)
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
#elif defined(HOME_BUTTONS_MINI)
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
#elif defined(HOME_BUTTONS_PRO)
  // pro only has powered awake mode
  device_state_.flags().awake_mode = true;
#endif

  // ------ read sensors ------
  hw_.read_temp_hmd(device_state_.sensors().temperature,
                    device_state_.sensors().humidity,
                    device_state_.get_use_fahrenheit());
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
  device_state_.sensors().battery_pct = hw_.read_battery_percent();
  device_state_.sensors().battery_voltage = hw_.read_battery_voltage();
#endif

  // ------ boot cause ------
  std::tie(boot_cause_, wakeup_btn_id_) = _determine_boot_cause();

  // ------ handle boot cause ------
  switch (boot_cause_) {
    case BootCause::RESET: {
      if (!device_state_.persisted().silent_restart) {
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
        hw_.set_all_leds(hw_.LED_BRIGHT_DFLT);
#elif defined(HOME_BUTTONS_PRO)
        hw_.set_frontlight(hw_.FL_LED_BRIGHT_DFLT);
#endif
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
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
        hw_.set_all_leds(0);
#elif defined(HOME_BUTTONS_PRO)
        hw_.set_frontlight(0);
#endif
      } else {
        display_.end();
        display_.update();
        _go_to_sleep();
      }
      break;
    }
    case BootCause::BUTTON: {
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
      if (!device_state_.flags().awake_mode) {
        if (device_state_.persisted().charge_complete_showing) {
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
          // proceed
        }
      } else {
        // proceed with awake mode
      }
      break;
#elif defined(HOME_BUTTONS_PRO)
      if (!device_state_.persisted().wifi_done) {
        device_state_.persisted().silent_restart = true;
        device_state_.persisted().restart_to_wifi_setup = true;
        device_state_.save_all();
        ESP.restart();
      } else if (!device_state_.persisted().setup_done) {
        device_state_.persisted().restart_to_setup = true;
        device_state_.persisted().silent_restart = true;
        device_state_.save_all();
        ESP.restart();
      }
#endif
    }

#if defined(HOME_BUTTONS_ORIGINAL)
    case BootCause::TIMER: {
      if (device_state_.flags().awake_mode) {
        // proceed with awake mode
      } else {
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
#elif defined(HOME_BUTTONS_MINI)
    case BootCause::TIMER: {
      break;
    }
#endif
    default:
      break;
  }

  display_.init_ui_state(UIState{.page = DisplayPage::MAIN});
  network_.set_mqtt_callback(std::bind(&App::_mqtt_callback, this,
                                       std::placeholders::_1,
                                       std::placeholders::_2));
  network_.set_on_connect(std::bind(&App::_net_on_connect, this));

#if defined(HOME_BUTTONS_PRO)
  touch_handler_.SetEventCallbackSecondary(
      std::bind(&App::_handle_ui_event_global, this, std::placeholders::_1));
#endif

  debug("Starting main loop");
  while (true) {
    loop();
    esp_task_wdt_reset();
    delay(10);
  }
}

void App::_handle_ui_event_global(UserInput::Event event) {
  device_state_.flags().last_user_input_time = millis();
}

void App::_publish_btn_event(UserInput::Event event) {
  TopicType topic = mqtt_.get_button_topic(event);
  network_.publish(topic, BTN_PRESS_PAYLOAD);
}

void App::_publish_sensors() {
  network_.publish(mqtt_.t_temperature(),
                   PayloadType("%.2f", device_state_.sensors().temperature));
  network_.publish(mqtt_.t_humidity(),
                   PayloadType("%.2f", device_state_.sensors().humidity));
  network_.publish(mqtt_.t_battery(),
                   PayloadType("%u", device_state_.sensors().battery_pct));
}

#if defined(HOME_BUTTONS_ORIGINAL)
void App::_publish_awake_mode_avlb() {
  if (hw_.is_dc_connected()) {
    network_.publish(mqtt_.t_awake_mode_avlb(), "online", true);
  } else {
    network_.publish(mqtt_.t_awake_mode_avlb(), "offline", true);
  }
}
#endif

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
#if defined(HOME_BUTTONS_ORIGINAL)
  _publish_awake_mode_avlb();
#endif
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
  if (device_state_.persisted().download_mdi_icons) {
    device_state_.persisted().download_mdi_icons = false;
    _download_mdi_icons();
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
  sm()._start_ui_task();
  sm()._start_display_task();
  sm()._start_network_task();

#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
  sm().buttons_.Start();
  sm().buttons_.InitPress(sm().wakeup_btn_id_);
  sm().leds_.Start();
#elif defined(HOME_BUTTONS_PRO)
  sm().touch_handler_.Start();
#endif

#if defined(HOME_BUTTONS_ORIGINAL)
  sm().mdi_.add_size(64);
  sm().mdi_.add_size(48);
#elif defined(HOME_BUTTONS_MINI)
  sm().mdi_.add_size(100);
#elif defined(HOME_BUTTONS_PRO)
  sm().mdi_.add_size(92);
  sm().mdi_.add_size(64);
#endif

  if (!sm().device_state_.flags().awake_mode) {
    esp_task_wdt_init(WDT_TIMEOUT_SLEEP, true);
    esp_task_wdt_add(NULL);
    if (sm().boot_cause_ == BootCause::TIMER ||
        sm().boot_cause_ == BootCause::RESET) {
      return transition_to<NetConnectingState>();
    } else {  // button
      return transition_to<SleepModeHandleInput>();
    }
  } else {
    sm().device_state_.persisted().check_connection = false;
    sm().device_state_.persisted().charge_complete_showing = false;
    esp_task_wdt_init(WDT_TIMEOUT_AWAKE, true);
    esp_task_wdt_add(NULL);
    sm().display_.disp_main();
    return transition_to<AwakeModeIdleState>();
  }
}

void AppSMStates::AwakeModeIdleState::entry() {
  sm().display_.disp_main();
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
  sm().buttons_.SetEventCallback(std::bind(&AwakeModeIdleState::handle_ui_event,
                                           this, std::placeholders::_1));
#elif defined(HOME_BUTTONS_PRO)
  sm().touch_handler_.SetEventCallback(std::bind(
      &AwakeModeIdleState::handle_ui_event, this, std::placeholders::_1));
#endif
}

void AppSMStates::AwakeModeIdleState::loop() {
  if (millis() - sm().last_sensor_publish_ >= AWAKE_SENSOR_INTERVAL) {
    sm().hw_.read_temp_hmd(sm().device_state_.sensors().temperature,
                           sm().device_state_.sensors().humidity,
                           sm().device_state_.get_use_fahrenheit());

#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
    sm().device_state_.sensors().battery_pct = sm().hw_.read_battery_percent();
    sm().device_state_.sensors().battery_voltage =
        sm().hw_.read_battery_voltage();
#endif
    sm()._publish_sensors();
    sm().last_sensor_publish_ = millis();
#ifdef HOME_BUTTONS_DEBUG
    sm()._log_task_stats();
#endif
  } else if (millis() - sm().last_m_display_redraw_ >= AWAKE_REDRAW_INTERVAL) {
    if (sm().device_state_.flags().display_redraw) {
      sm().device_state_.flags().display_redraw = false;
      if (sm().device_state_.persisted().download_mdi_icons) {
        sm()._download_mdi_icons();
        sm().device_state_.persisted().download_mdi_icons = false;
      }
      sm().display_.disp_main();
    }
    sm().last_m_display_redraw_ = millis();
  }
#if defined(HOME_BUTTONS_PRO)
  else if (millis() - sm().device_state_.flags().last_user_input_time >
           FRONTLIGHT_TIMEOUT) {
    sm().hw_.set_frontlight(0);
  }
#endif
#if defined(HOME_BUTTONS_ORIGINAL)
  else if (!sm().hw_.is_dc_connected()) {
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
#endif
}

void AppSMStates::AwakeModeIdleState::handle_ui_event(UserInput::Event event) {
  sm().device_state_.flags().last_user_input_time = millis();
#if defined(HOME_BUTTONS_PRO)
  sm().hw_.set_frontlight(sm().hw_.FL_LED_BRIGHT_DFLT);
#endif
  if (event.final) {
    switch (event.type) {
      case UserInput::EventType::kClickSingle:
      case UserInput::EventType::kClickDouble:
      case UserInput::EventType::kClickTriple:
      case UserInput::EventType::kClickQuad:
        sm()._publish_btn_event(event);
        sm().leds_.Blink(event.btn_id,
                         UserInput::EventType2NumClicks(event.type),
                         sm().hw_.LED_BRIGHT_DFLT, 0, 0, false);
        break;
      case UserInput::EventType::kSwipeDown:
        return transition_to<InfoScreenState>();
      default:
        break;
    }
  } else {
    switch (event.type) {
      case UserInput::EventType::kHoldLong2s:
        if (sm().hw_.num_buttons_pressed() == 1) {
          sm().buttons_.Restart();
          return transition_to<InfoScreenState>();
        }
        break;
      case UserInput::EventType::kHoldLong5s:
        if (sm().hw_.num_buttons_pressed() == 2) {
          sm().buttons_.Restart();
          return transition_to<SettingsMenuState>();
        }
        break;
      default:
        break;
    }
  }
}

void AppSMStates::SleepModeHandleInput::entry() {
  sm().input_start_time_ = millis();
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
  sm().buttons_.SetEventCallback(std::bind(
      &SleepModeHandleInput::handle_ui_event, this, std::placeholders::_1));
#elif defined(HOME_BUTTONS_PRO)
  sm().touch_handler_.SetEventCallback(std::bind(
      &SleepModeHandleInput::handle_ui_event, this, std::placeholders::_1));
#endif
}

void AppSMStates::SleepModeHandleInput::loop() {
  if (millis() - sm().input_start_time_ > 10000UL) {
    return transition_to<CmdShutdownState>();
  }
}

void AppSMStates::SleepModeHandleInput::handle_ui_event(
    UserInput::Event event) {
  sm().device_state_.flags().last_user_input_time = millis();
  if (event.final) {
    switch (event.type) {
      case UserInput::EventType::kClickSingle:
      case UserInput::EventType::kClickDouble:
      case UserInput::EventType::kClickTriple:
      case UserInput::EventType::kClickQuad:
        if (!sm().device_state_.persisted().wifi_done) {
          sm().device_state_.persisted().restart_to_wifi_setup = true;
          sm().device_state_.persisted().silent_restart = true;
          sm().device_state_.save_all();
          sm().info("restarting to Wi-Fi setup...");
          ESP.restart();
        } else if (!sm().device_state_.persisted().setup_done) {
          sm().device_state_.persisted().restart_to_setup = true;
          sm().device_state_.persisted().silent_restart = true;
          sm().device_state_.save_all();
          sm().info("restarting to setup...");
          ESP.restart();
        }
        sm().leds_.Blink(event.btn_id,
                         UserInput::EventType2NumClicks(event.type),
                         sm().hw_.LED_BRIGHT_DFLT, 0, 0, true);
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
        if (sm().device_state_.sensors().battery_low) {
          sm().display_.disp_message_large(BATT_EMPTY_MSG, 3000);
        }
#endif
        sm().user_event_ = event;
        return transition_to<NetConnectingState>();
      default:
        break;
    }
  } else {  // non final event
    switch (event.type) {
      case UserInput::EventType::kHoldLong2s:
        if (sm().hw_.num_buttons_pressed() == 1) {
          sm().buttons_.Restart();
          return transition_to<InfoScreenState>();
        }
        break;
      case UserInput::EventType::kHoldLong5s:
        if (sm().hw_.num_buttons_pressed() == 2) {
          sm().buttons_.Restart();
          return transition_to<SettingsMenuState>();
        }
        break;
      default:
        break;
    }
  }
}

void AppSMStates::NetConnectingState::entry() {
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
  sm().buttons_.SetEventCallback(std::bind(&NetConnectingState::handle_ui_event,
                                           this, std::placeholders::_1));
#elif defined(HOME_BUTTONS_PRO)
  sm().touch_handler_.SetEventCallback(std::bind(
      &NetConnectingState::handle_ui_event, this, std::placeholders::_1));
#endif
}

void AppSMStates::NetConnectingState::loop() {
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
  if (sm().network_.get_state() == Network::State::M_CONNECTED) {
    if (sm().user_event_.type != UserInput::EventType::kNone) {
      sm()._publish_btn_event(sm().user_event_);
    }
    sm().hw_.read_temp_hmd(sm().device_state_.sensors().temperature,
                           sm().device_state_.sensors().humidity,
                           sm().device_state_.get_use_fahrenheit());
    sm().device_state_.sensors().battery_pct = sm().hw_.read_battery_percent();
    sm()._publish_sensors();
    sm().device_state_.persisted().failed_connections = 0;
    return transition_to<CmdShutdownState>();

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
#endif
}

void AppSMStates::NetConnectingState::handle_ui_event(UserInput::Event event) {
  sm().device_state_.flags().last_user_input_time = millis();
  if (event.final) {
    switch (event.type) {
      case UserInput::EventType::kClickSingle:
        sm().info("button press - user cancelled, aborting...");
        return transition_to<CmdShutdownState>();
        break;
      default:
        break;
    }
  }
}

void AppSMStates::InfoScreenState::entry() {
  sm().info_screen_start_time_ = millis();
  sm().display_.disp_info();
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
  sm().buttons_.SetEventCallback(std::bind(&InfoScreenState::handle_ui_event,
                                           this, std::placeholders::_1));
#elif defined(HOME_BUTTONS_PRO)
  sm().device_state_.flags().keep_frontlight_on = true;
  sm().hw_.set_frontlight(sm().hw_.FL_LED_BRIGHT_DFLT);
  sm().touch_handler_.SetEventCallback(std::bind(
      &InfoScreenState::handle_ui_event, this, std::placeholders::_1));
#endif
}

void AppSMStates::InfoScreenState::exit() {
  sm().device_state_.flags().keep_frontlight_on = false;
}

void AppSMStates::InfoScreenState::loop() {
  if (millis() - sm().info_screen_start_time_ >= INFO_SCREEN_DISP_TIME) {
    sm().debug("info screen timeout");
    if (sm().device_state_.flags().awake_mode) {
      return transition_to<AwakeModeIdleState>();
    } else {
      sm().display_.disp_main();
      return transition_to<CmdShutdownState>();
    }
  }
}

void AppSMStates::InfoScreenState::handle_ui_event(UserInput::Event event) {
  sm().device_state_.flags().last_user_input_time = millis();
  if (event.final) {
    switch (event.type) {
      case UserInput::EventType::kSwipeUp:
      case UserInput::EventType::kSwipeDown:
      case UserInput::EventType::kClickSingle:
        if (sm().device_state_.flags().awake_mode) {
          return transition_to<AwakeModeIdleState>();
        } else {
          sm().display_.disp_main();
          return transition_to<CmdShutdownState>();
        }
      default:
        break;
    }
  }
}

void AppSMStates::SettingsMenuState::entry() {
  sm().settings_menu_start_time_ = millis();
  sm().display_.disp_settings();

#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
  sm().buttons_.SetEventCallback(std::bind(&SettingsMenuState::handle_ui_event,
                                           this, std::placeholders::_1));
#elif defined(HOME_BUTTONS_PRO)
  sm().device_state_.flags().keep_frontlight_on = true;
  sm().hw_.set_frontlight(sm().hw_.FL_LED_BRIGHT_DFLT);
  sm().touch_handler_.SetEventCallback(std::bind(
      &SettingsMenuState::handle_ui_event, this, std::placeholders::_1));
#endif
}

void AppSMStates::SettingsMenuState::exit() {
  sm().device_state_.flags().keep_frontlight_on = false;
}

void AppSMStates::SettingsMenuState::loop() {
  if (millis() - sm().settings_menu_start_time_ > SETTINGS_MENU_TIMEOUT) {
    sm().debug("settings menu timeout");
    if (sm().device_state_.flags().awake_mode) {
      return transition_to<AwakeModeIdleState>();
    } else {
      return transition_to<CmdShutdownState>();
    }
  }
}

void AppSMStates::SettingsMenuState::handle_ui_event(UserInput::Event event) {
  sm().device_state_.flags().last_user_input_time = millis();
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
  if (event.final) {
    switch (event.type) {
      case UserInput::EventType::kClickSingle:
        switch (event.btn_id) {
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
            if (sm().device_state_.flags().awake_mode) {
              return transition_to<AwakeModeIdleState>();
            } else {
              sm().display_.disp_main();
              return transition_to<CmdShutdownState>();
            }
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
  } else {  // non final
    switch (event.type) {
      case UserInput::EventType::kHoldLong10s:
        if (event.btn_id == 3) {
          sm().buttons_.Restart();
          // factory reset
          return transition_to<FactoryResetState>();
        }
        break;
      case UserInput::EventType::kHoldLong2s:
        if (event.btn_id == 1) {
          sm().buttons_.Restart();
          // device info screen
          return transition_to<DeviceInfoState>();
        }
        break;
      default:
        break;
    }
  }

#elif defined(HOME_BUTTONS_PRO)
  if (event.final) {
    switch (event.type) {
      case UserInput::EventType::kClickSingle:
        if (event.point.y < 74) {
          // setup
          sm().device_state_.persisted().restart_to_setup = true;
          sm().device_state_.persisted().silent_restart = true;
          sm().device_state_.save_all();
          ESP.restart();
        } else if (event.point.y < 149) {
          // Wi-Fi setup
          sm().device_state_.persisted().restart_to_wifi_setup = true;
          sm().device_state_.persisted().silent_restart = true;
          sm().device_state_.save_all();
          ESP.restart();
        } else if (event.point.y < 224) {
          // restart
          ESP.restart();
        } else {
          // exit
          sm().display_.disp_main();
          return transition_to<AwakeModeIdleState>();
        }
        break;
      case UserInput::EventType::kHoldLong10s:
        if (event.point.y > 149 && event.point.y < 224) {
          // factory reset
          return transition_to<FactoryResetState>();
        }
        break;
      default:
        break;
    }
  }
#endif
}

void AppSMStates::DeviceInfoState::entry() {
  sm().device_info_start_time_ = millis();
  sm().display_.disp_device_info();
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
  sm().buttons_.SetEventCallback(std::bind(&DeviceInfoState::handle_ui_event,
                                           this, std::placeholders::_1));
#elif defined(HOME_BUTTONS_PRO)
  sm().device_state_.flags().keep_frontlight_on = true;
  sm().hw_.set_frontlight(sm().hw_.FL_LED_BRIGHT_DFLT);
  sm().touch_handler_.SetEventCallback(std::bind(
      &DeviceInfoState::handle_ui_event, this, std::placeholders::_1));
#endif
}

void AppSMStates::DeviceInfoState::loop() {
  if (millis() - sm().device_info_start_time_ > DEVICE_INFO_TIMEOUT) {
    sm().debug("device info timeout");
    if (sm().device_state_.flags().awake_mode) {
      return transition_to<AwakeModeIdleState>();
    } else {
      sm().display_.disp_main();
      return transition_to<CmdShutdownState>();
    }
  }
}

void AppSMStates::DeviceInfoState::handle_ui_event(UserInput::Event event) {
  sm().device_state_.flags().last_user_input_time = millis();
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
  if (event.final) {
    switch (event.type) {
      case UserInput::EventType::kClickSingle:
        // cancel
        if (sm().device_state_.flags().awake_mode) {
          return transition_to<AwakeModeIdleState>();
        } else {
          sm().display_.disp_main();
          return transition_to<CmdShutdownState>();
        }
        break;
      default:
        break;
    }
  }
#elif defined(HOME_BUTTONS_PRO)
#TODO
#endif
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
    sm().display_.disp_main();
  }
  sm().shutdown_cmd_time_ = millis();
}

void AppSMStates::CmdShutdownState::loop() {
  // wait for timeout
  if (millis() - sm().shutdown_cmd_time_ > SHUTDOWN_DELAY) {
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
    sm().buttons_.Stop();
    sm().leds_.Stop();
#elif defined(HOME_BUTTONS_PRO)
    sm().touch_handler_.Stop();
#endif
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
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
  bool ended =
      sm().display_.get_state() == Display::State::IDLE &&
      sm().leds_.cstate() == ComponentBase::ComponentState::kStopped &&
      sm().buttons_.cstate() == ComponentBase::ComponentState::kStopped &&
      !sm().hw_.any_button_pressed();
#elif defined(HOME_BUTTONS_PRO)
  bool ended =
      sm().display_.get_state() == Display::State::IDLE &&
      sm().touch_handler_.cstate() == ComponentBase::ComponentState::kStopped;
#endif

  if (ended) {
#ifdef HOME_BUTTONS_DEBUG
    sm()._log_task_stats();
#endif
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
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
  sm().buttons_.Stop();
#elif defined(HOME_BUTTONS_PRO)
#endif
}

void AppSMStates::FactoryResetState::loop() {
  if (sm().network_.get_state() == Network::State::DISCONNECTED &&
      sm().display_.get_state() == Display::State::IDLE) {
    sm().device_state_.clear_all();
    sm().info("factory reset complete.");
    ESP.restart();
  }
}

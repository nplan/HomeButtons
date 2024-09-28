#include "app.h"

#include <Arduino.h>
#include <esp_task_wdt.h>
#include <SPIFFS.h>
#include "esp_ota_ops.h"
#include <ArduinoJson.h>

#include "config.h"
#include "factory.h"
#include "hardware.h"

extern "C" bool verifyRollbackLater() { return true; };

App::App()
    : AppStateMachine("AppSM", *this),
      Logger("APP"),
#if defined(HOME_BUTTONS_ORIGINAL)
      b1_("B1", 1, false, true, hw_),
      b2_("B2", 2, false, true, hw_),
      b3_("B3", 3, false, true, hw_),
      b4_("B4", 4, false, true, hw_),
      b5_("B5", 5, false, true, hw_),
      b6_("B6", 6, false, true, hw_),
      bsl_input_("BSLInput",
                 std::array<std::reference_wrapper<BtnSwLED>, NUM_BUTTONS>{
                     b1_, b2_, b3_, b4_, b5_, b6_}),
#elif defined(HOME_BUTTONS_MINI)
      b1_("B1", 1, false, true, hw_),
      b2_("B2", 2, false, true, hw_),
      b3_("B3", 3, false, true, hw_),
      b4_("B4", 4, false, true, hw_),
      bsl_input_("BSLInput",
                 std::array<std::reference_wrapper<BtnSwLED>, NUM_BUTTONS>{
                     b1_, b2_, b3_, b4_}),
#elif defined(HOME_BUTTONS_PRO)
      touch_handler_(hw_),
#elif defined(HOME_BUTTONS_INDUSTRIAL)
      b1_("B1", 1, false, true, hw_),
      b2_("B2", 2, false, true, hw_),
      b3_("B3", 3, false, true, hw_),
      b4_("B4", 4, false, true, hw_),
      sw_("SW", 5, true, false, hw_),
      bsl_input_("BSLInput",
                 std::array<std::reference_wrapper<BtnSwLED>, NUM_BUTTONS>{
                     b1_, b2_, b3_, b4_, sw_}),
#endif
#if defined(HAS_DISPLAY)
      display_(device_state_, mdi_),
#endif
      topics_(device_state_),
      network_(device_state_, topics_),
      mqtt_(device_state_, bsl_input_, network_, topics_),
      setup_(*this) {
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

void App::_sleep_or_restart() {
  delay(3000);
#if defined(HAS_SLEEP_MODE)
  error("Going to sleep...");
  _start_esp_sleep();
#else
  error("Restarting...");
  ESP.restart();
#endif
}

#if defined(HAS_SLEEP_MODE)
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
#endif

std::pair<BootCause, int16_t> App::_determine_boot_cause() {
  BootCause boot_cause = BootCause::RESET;
  int16_t wakeup_pin = 0;
  uint8_t wakeup_btn_id = 0;
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_EXT1: {
      uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
      wakeup_pin = (log(GPIO_reason)) / log(2);
      debug("wakeup cause: PIN %d", wakeup_pin);
      wakeup_btn_id = bsl_input_.IdFromPin(wakeup_pin);
      if (wakeup_btn_id > 0) {
        boot_cause = BootCause::BUTTON;
      } else {
        boot_cause = BootCause::RESET;
      }
    } break;
    case ESP_SLEEP_WAKEUP_TIMER:
      boot_cause = BootCause::TIMER;
      break;
    default:
      boot_cause = BootCause::RESET;
      break;
  }
#elif defined(HOME_BUTTONS_PRO) || defined(HOME_BUTTONS_INDUSTRIAL)
  boot_cause = BootCause::RESET;
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

void App::_publish_system_state() {
  uint32_t esp_free_heap = ESP.getFreeHeap();
  uint32_t esp_min_free_heap = ESP.getMinFreeHeap();
  uint32_t uptime = millis() / 1000;
  int32_t rssi = network_.get_rssi();
  IPAddress ip = network_.get_ip();

  StaticJsonDocument<512> doc;
  doc["esp_free_heap"] = esp_free_heap;
  doc["esp_min_free_heap"] = esp_min_free_heap;
  doc["uptime_seconds"] = uptime;
  doc["wifi_rssi"] = rssi;
  doc["ip_address"] = ip.toString();
  doc["sw_version"] = SW_VERSION;
#if defined(HAS_BATTERY)
  doc["batt_voltage"] = hw_.read_battery_voltage();
#endif

  char buffer[512];
  serializeJson(doc, buffer, sizeof(buffer));
  network_.publish(topics_.t_system_state(), buffer, true);
}

void App::_ui_task(void* param) {
  App* app = static_cast<App*>(param);
  while (true) {
#if defined(HAS_BUTTON_UI)
    app->bsl_input_.Loop();
#elif defined(HAS_TOUCH_UI)
    app->touch_handler_.Loop();
#endif

#if defined(HAS_FRONTLIGHT)
    if (millis() - app->device_state_.flags().last_user_input_time >
        FRONTLIGHT_TIMEOUT) {
      if (!app->device_state_.flags().keep_frontlight_on) {
        app->hw_.set_frontlight(0);
      }
    }
#endif
    delay(5);
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

#if defined(HAS_DISPLAY)
void App::_display_task(void* param) {
  App* app = static_cast<App*>(param);
  while (true) {
    app->display_.update();
    delay(50);
  }
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
#endif

void App::_network_task(void* param) {
  App* app = static_cast<App*>(param);
  app->network_.setup();
  while (true) {
    app->network_.update();
    delay(10);
  }
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

void App::_begin_hw() {
#if defined(HAS_DISPLAY)
  // must be before ledAttachPin (reserves GPIO37 = SPIDQS)
  display_.begin(hw_);
#endif
  hw_.begin();
#if defined(HAS_BUTTON_UI)
  bsl_input_.Init();
  bsl_input_.LEDSetDefaultBrightnessAll(LED_DFLT_BRIGHT);
#elif defined(HAS_TOUCH_UI)
  touch_handler_.Init(hw_.TOUCH_CLICK_PIN, hw_.TOUCH_INT_PIN);
#endif

#if defined(HOME_BUTTONS_INDUSTRIAL)
  // set button config based
  auto conf = device_state_.user_preferences().btn_conf_string;
  debug("Button config: %s", conf.c_str());
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    if (conf[i] == 'S') {
      bsl_input_.SetSwitchMode(i + 1, true);
    }
  }
  sw_.SetSwitchMode(true);
#endif
}

void App::_start_tasks() {
  _start_ui_task();
  _start_network_task();
#if defined(HAS_DISPLAY)
  _start_display_task();
#endif

#if defined(HAS_BUTTON_UI)
  bsl_input_.Start();
#elif defined(HAS_TOUCH_UI)
  touch_handler_.Start();
#endif
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
    error("HW init failed!");
    _sleep_or_restart();
  }

  // ------ factory test ------
  FactoryTest factory_test(*this);
  if (factory_test.is_test_required()) {
    if (factory_test.run_test()) {
      device_state_.load_all(hw_);
#if defined(HAS_DISPLAY)
      display_.disp_welcome();
      display_.update();
#else
      bsl_input_.LEDBlinkAll(2, LED_DFLT_BRIGHT, 500, 400, false);
#endif
      info("factory test complete.");
      _sleep_or_restart();
    } else {
      error("factory test failed!");
#if defined(HAS_DISPLAY)
      display_.disp_error("Factory\nTest\nFailed");
      display_.update();
#else
      bsl_input_.LEDBlinkAll(5, LED_DFLT_BRIGHT, 200, 160, false);
#endif
      _sleep_or_restart();
    }
  }

  device_state_.load_all(hw_);
  _begin_hw();

  // ------ test code ------

  // place test code here
  // info("!!!!! Serial print test");
  // Serial.println("Serial");
  // Serial1.println("Serial1");
  // Serial.begin(115200);
  // Serial1.begin(115200);
  // Serial.println("Serial after begin");
  // Serial1.println("Serial1 after begin");
  // debug("Debug");

  // while (true) {
  //   delay(1000);
  // }

  // ------ after update handler ------
  if (device_state_.persisted().last_sw_ver != SW_VERSION) {
    if (device_state_.persisted().last_sw_ver.length() > 0) {
      info("firmware updated from %s to %s",
           device_state_.persisted().last_sw_ver.c_str(), SW_VERSION);
      device_state_.persisted().last_sw_ver = SW_VERSION;
      device_state_.persisted().send_discovery_config = true;
      device_state_.save_all();
#if defined(HAS_DISPLAY)
      display_.disp_message(
          (UIState::MessageType("Firmware\nupdated to\n") + SW_VERSION)
              .c_str());
      display_.update();
#endif
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
#elif defined(HOME_BUTTONS_PRO) || defined(HOME_BUTTONS_INDUSTRIAL)
  device_state_.flags().awake_mode = true;
#endif

#if defined(HAS_TH_SENSOR)
  // ------ read sensors ------
  hw_.read_temp_hmd(device_state_.sensors().temperature,
                    device_state_.sensors().humidity,
                    device_state_.get_use_fahrenheit());
#endif
#if defined(HAS_BATTERY)
  device_state_.sensors().battery_pct = hw_.read_battery_percent();
  device_state_.sensors().battery_voltage = hw_.read_battery_voltage();
#endif

  // ------ start tasks ------
  _start_tasks();

  // ------ boot cause ------
  std::tie(boot_cause_, wakeup_btn_id_) = _determine_boot_cause();
  info("boot cause: %d, wakeup btn id: %d", static_cast<int>(boot_cause_),
       wakeup_btn_id_);

  // ------ handle boot cause ------
  switch (boot_cause_) {
    case BootCause::RESET: {
      if (!device_state_.persisted().silent_restart) {
#if defined(HAS_BUTTON_UI)
        bsl_input_.LEDOnAll();
        delay(1000);
#endif
#if defined(HAS_FRONTLIGHT)
        hw_.set_frontlight(hw_.FL_LED_BRIGHT_DFLT);
#endif
#if defined(HAS_DISPLAY)
        display_.disp_message("RESTART...", 0);
        delay(3000);
#endif
      }

#if defined(HAS_DISPLAY)
      // format SPIFFS if needed
      if (!SPIFFS.begin()) {
        info("Formatting icon storage...");
        display_.disp_message("Formatting\nIcon\nStorage...", 0);
        delay(3000);
        SPIFFS.format();
      } else {
        SPIFFS.end();
        debug("SPIFFS test mount OK");
      }
#endif

      // check if restart to setup or Wi-Fi setup is needed
      if (device_state_.persisted().restart_to_wifi_setup) {
        device_state_.clear_persisted_flags();
        info("staring Wi-Fi setup...");
        setup_.start_wifi_setup();  // resets ESP when done
      } else if (device_state_.persisted().restart_to_setup) {
        device_state_.clear_persisted_flags();
        info("staring setup...");
        setup_.start_setup();  // resets ESP when done
      }

      device_state_.clear_persisted_flags();

      if (!device_state_.persisted().wifi_done ||
          !device_state_.persisted().setup_done) {
#if defined(HAS_DISPLAY)
        display_.disp_welcome();
        delay(3000);
        display_.end();
        delay(3000);
#endif
#if defined(HAS_SLEEP_MODE)
        _go_to_sleep();
#endif
      } else {
#if defined(HAS_DISPLAY)
        display_.disp_main();
        delay(3000);
#endif
      }
      device_state_.persisted().download_mdi_icons = true;
      device_state_.persisted().send_discovery_config = true;
      device_state_.save_all();
      if (device_state_.flags().awake_mode) {
// proceed with awake mode
#if defined(HAS_BUTTON_UI)
        bsl_input_.LEDOffAll();
#elif defined(HAS_FRONTLIGHT)
        hw_.set_frontlight(0);
#endif
      } else {
#if defined(HAS_DISPLAY)
        display_.end();
        delay(3000);
#endif
#if defined(HAS_SLEEP_MODE)
        _go_to_sleep();
#endif
      }
      break;
    }
    case BootCause::BUTTON: {
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
      if (!device_state_.flags().awake_mode) {
        if (device_state_.persisted().charge_complete_showing) {
          device_state_.persisted().charge_complete_showing = false;
          display_.disp_main();
          delay(3000);
          display_.end();
          delay(3000);
          _go_to_sleep();
        } else if (device_state_.persisted().user_msg_showing) {
          device_state_.persisted().user_msg_showing = false;
          display_.disp_main();
          delay(3000);
          display_.end();
          delay(3000);
          _go_to_sleep();
        } else if (device_state_.persisted().check_connection) {
          device_state_.persisted().check_connection = false;
          display_.disp_main();
          delay(3000);
          display_.end();
          delay(3000);
          _go_to_sleep();
        } else {
          // proceed
        }
      } else {
        // proceed with awake mode
      }
      break;
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
#endif
    default:
      break;
  }

#if defined(HAS_DISPLAY)
  display_.init_ui_state(UIState{.page = DisplayPage::MAIN});
#endif
  network_.set_mqtt_callback(std::bind(&App::_mqtt_callback, this,
                                       std::placeholders::_1,
                                       std::placeholders::_2));
  network_.set_on_connect(std::bind(&App::_net_on_connect, this));

#if defined(HAS_TOUCH_UI)
  touch_handler_.SetEventCallbackSecondary(
      std::bind(&App::_handle_ui_event_global, this, std::placeholders::_1));
#endif

  debug("Starting main state machine loop");
  while (true) {
    loop();
    esp_task_wdt_reset();
    delay(10);
  }
}

void App::_handle_ui_event_global(UserInput::Event event) {
  device_state_.flags().last_user_input_time = millis();
}

void App::_publish_ui_event(UserInput::Event event) {
  TopicType topic = topics_.get_button_topic(event);
  if (event.type == UserInput::EventType::kClickSingle ||
      event.type == UserInput::EventType::kClickDouble ||
      event.type == UserInput::EventType::kClickTriple ||
      event.type == UserInput::EventType::kClickQuad) {
    network_.publish(topic, BTN_PRESS_PAYLOAD);
  } else if (event.type == UserInput::EventType::kSwitchOn) {
    network_.publish(topic, "ON");
  } else if (event.type == UserInput::EventType::kSwitchOff) {
    network_.publish(topic, "OFF");
  }
}

#if defined(HAS_TH_SENSOR)
void App::_publish_sensors() {
  network_.publish(topics_.t_temperature(),
                   PayloadType("%.2f", device_state_.sensors().temperature));
  network_.publish(topics_.t_humidity(),
                   PayloadType("%.2f", device_state_.sensors().humidity));
  network_.publish(topics_.t_battery(),
                   PayloadType("%u", device_state_.sensors().battery_pct));
}
#endif

#if defined(HAS_BATTERY)
void App::_publish_battery() {
  network_.publish(topics_.t_battery(),
                   PayloadType("%u", device_state_.sensors().battery_pct));
}
#endif

#if defined(HAS_AWAKE_MODE)
void App::_publish_awake_mode_avlb() {
  if (hw_.is_dc_connected()) {
    network_.publish(topics_.t_awake_mode_avlb(), "online", true);
  } else {
    network_.publish(topics_.t_awake_mode_avlb(), "offline", true);
  }
}
#endif

void App::_mqtt_callback(const char* topic, const char* payload) {
#if defined(HAS_TH_SENSOR)
  if (strcmp(topic, topics_.t_sensor_interval_cmd().c_str()) == 0) {
    uint16_t mins = atoi(payload);
    if (mins >= SEN_INTERVAL_MIN && mins <= SEN_INTERVAL_MAX) {
      device_state_.set_sensor_interval(mins);
      device_state_.save_all();
      network_.publish(topics_.t_sensor_interval_state(),
                       PayloadType("%u", device_state_.sensor_interval()),
                       true);
      info("Updating discovery config...");
      mqtt_.update_discovery_config();
      debug("sensor interval set to %d minutes", mins);
      _publish_sensors();
    }
    network_.publish(topics_.t_sensor_interval_cmd(), "", true);
    return;
  }
#endif

#if defined(HAS_DISPLAY)
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    if (strcmp(topic, topics_.t_btn_label_cmd(i + 1).c_str()) == 0) {
      ButtonLabel new_label(payload);
      new_label = new_label.trim();
      debug("button %d label changed to: %s", i + 1, new_label.c_str());
      device_state_.set_btn_label(i + 1, new_label.c_str());

      network_.publish(topics_.t_btn_label_state(i + 1),
                       device_state_.get_btn_label(i + 1), true);
      network_.publish(topics_.t_btn_label_cmd(i + 1), "", true);
      device_state_.flags().display_redraw = true;
      device_state_.save_all();

      ButtonLabel label(device_state_.get_btn_label(i + 1).c_str());

      if (label.substring(0, 4) == "mdi:") {
        device_state_.persisted().download_mdi_icons = true;
      }
      return;
    }
  }
#endif

#if defined(HAS_AWAKE_MODE)
  if (strcmp(topic, topics_.t_awake_mode_cmd().c_str()) == 0) {
    if (strcmp(payload, "ON") == 0) {
      device_state_.persisted().user_awake_mode = true;
      device_state_.flags().awake_mode = true;
      device_state_.save_all();
      network_.publish(topics_.t_awake_mode_state(), "ON", true);
      debug("user awake mode set to: ON");
      debug("resetting to awake mode...");
    } else if (strcmp(payload, "OFF") == 0) {
      device_state_.persisted().user_awake_mode = false;
      device_state_.save_all();
      network_.publish(topics_.t_awake_mode_state(), "OFF", true);
      debug("user awake mode set to: OFF");
    }
    network_.publish(topics_.t_awake_mode_cmd(), "", true);
    return;
  }
#endif

#if defined(HAS_DISPLAY)
  // user message
  if (strcmp(topic, topics_.t_disp_msg_cmd().c_str()) == 0) {
    if (display_.get_ui_state().page == DisplayPage::MAIN) {
      UserMessage msg(payload);
      device_state_.persisted().user_msg_showing = true;
      device_state_.save_all();
      display_.disp_message_large(msg.c_str());
    }
    network_.publish(topics_.t_disp_msg_cmd(), "", true);
    network_.publish(topics_.t_disp_msg_state(), "-", false);
  }
#endif

#if defined(HAS_SLEEP_MODE)
  // schedule wakeup cmd
  if (strcmp(topic, topics_.t_schedule_wakeup_cmd().c_str()) == 0) {
    uint32_t secs = atoi(payload);
    if (secs >= SCHEDULE_WAKEUP_MIN && secs <= SCHEDULE_WAKEUP_MAX) {
      device_state_.flags().schedule_wakeup_time = secs;
      network_.publish(topics_.t_schedule_wakeup_cmd(), "", true);
      network_.publish(topics_.t_schedule_wakeup_state(), "None", true);
      debug("schedule wakeup set to %d seconds", secs);
    }
  }
#endif

#if defined(HOME_BUTTONS_INDUSTRIAL)
  // led amb_bright cmd
  if (strcmp(topic, topics_.t_led_amb_bright_cmd().c_str()) == 0) {
    uint16_t amb_bright = atoi(payload);
    if (amb_bright <= LED_MAX_AMB_BRIGHT) {
      device_state_.set_led_brightness(amb_bright);
      device_state_.save_all();
      bsl_input_.LEDSetAmbientBrightnessAll(amb_bright);
      bsl_input_.LEDSetDefaultBrightnessAll(
          amb_bright * (LED_DFLT_BRIGHT / LED_MAX_AMB_BRIGHT));
      network_.publish(topics_.t_led_amb_bright_state(),
                       PayloadType("%u", amb_bright), true);
      debug("LED amb_bright set to %d", amb_bright);
    } else {
      warning("Invalid amb_bright value: %d", amb_bright);
    }
    network_.publish(topics_.t_led_amb_bright_cmd(), "", true);
    return;
  }

  // switch cmd
  for (auto bsl_w : bsl_input_.GetBtnSwLEDs()) {
    if (bsl_w.get().switch_mode() && !bsl_w.get().is_kill_switch()) {
      if (strcmp(topic, topics_.t_switch_cmd(bsl_w.get().id()).c_str()) == 0) {
        if (strcmp(payload, "ON") == 0) {
          bsl_w.get().SetSwitchOn();
          network_.publish(topics_.t_switch_state(bsl_w.get().id()), "ON",
                           false);
        } else if (strcmp(payload, "OFF") == 0) {
          network_.publish(topics_.t_switch_state(bsl_w.get().id()), "OFF",
                           false);
          bsl_w.get().SetSwitchOff();
        }
        network_.publish(topics_.t_switch_cmd(bsl_w.get().id()), "", true);
        return;
      }
    }
  }
#endif
}

void App::_net_on_connect() {
  if (device_state_.persisted().send_discovery_config) {
    device_state_.persisted().send_discovery_config = false;
    info("Sending discovery config...");
    mqtt_.send_discovery_config();
  }

  network_.subscribe(topics_.t_cmd() + "#");
#if defined(HAS_AWAKE_MODE)
  _publish_awake_mode_avlb();
  network_.publish(topics_.t_awake_mode_state(),
                   (device_state_.persisted().user_awake_mode) ? "ON" : "OFF",
                   true);
#endif
#if defined(HAS_TH_SENSOR)
  network_.publish(topics_.t_sensor_interval_state(),
                   PayloadType("%u", device_state_.sensor_interval()), true);
#endif
#if defined(HAS_DISPLAY)
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    auto t = topics_.t_btn_label_state(i + 1);
    network_.publish(t, device_state_.get_btn_label(i + 1), true);
  }
  network_.publish(topics_.t_disp_msg_state(), "-", false);
#endif

#if defined(HOME_BUTTONS_INDUSTRIAL)
  network_.publish(
      topics_.t_led_amb_bright_state(),
      PayloadType("%u", device_state_.user_preferences().led_amb_bright, true));
  network_.publish(topics_.t_avlb(), "online", true);
  for (auto bsl_w : bsl_input_.GetBtnSwLEDs()) {
    // publish switch state is switch mode
    if (bsl_w.get().switch_mode()) {
      network_.publish(topics_.t_switch_state(bsl_w.get().id()),
                       bsl_w.get().switch_state() ? "ON" : "OFF", false);
    }
  }
#endif

#if defined(HAS_DISPLAY)
  if (device_state_.persisted().download_mdi_icons) {
    device_state_.persisted().download_mdi_icons = false;
    _download_mdi_icons();
  }
#endif
}

#if defined(HAS_DISPLAY)
void App::_download_mdi_icons() {
  bool download_required = false;
  mdi_.begin();
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    ButtonLabel label(device_state_.get_btn_label(i + 1).c_str());
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
    ButtonLabel label(device_state_.get_btn_label(i + 1).c_str());
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
#endif

void AppSMStates::InitState::entry() {
  sm().network_.connect();
  sm().bsl_input_.InitPress(sm().wakeup_btn_id_);

#if defined(HOME_BUTTONS_ORIGINAL)
  sm().mdi_.add_size(64);
  sm().mdi_.add_size(48);
#elif defined(HOME_BUTTONS_MINI)
  sm().mdi_.add_size(100);
#elif defined(HOME_BUTTONS_PRO)
  sm().mdi_.add_size(92);
  sm().mdi_.add_size(64);
#endif

#if defined(HOME_BUTTONS_INDUSTRIAL)
  uint8_t amb_bright = sm().device_state_.user_preferences().led_amb_bright;
  sm().bsl_input_.LEDSetAmbientBrightnessAll(amb_bright);
  uint8_t dflt_bright = amb_bright * (LED_DFLT_BRIGHT / LED_MAX_AMB_BRIGHT);
  sm().bsl_input_.LEDSetDefaultBrightnessAll(dflt_bright);
#endif

  // open settings menu if setup not done
  if (sm().device_state_.flags().awake_mode) {
    if (!sm().device_state_.persisted().wifi_done) {
      sm().info("Wi-Fi setup not done, opening settings menu...");
      return transition_to<SettingsMenuState>();
    } else if (!sm().device_state_.persisted().setup_done) {
      sm().info("setup not done, opening settings menu...");
      return transition_to<SettingsMenuState>();
    }
  }

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
#if defined(HAS_DISPLAY)
    sm().display_.disp_main();
#endif
    return transition_to<AwakeModeIdleState>();
  }
}

void AppSMStates::AwakeModeIdleState::entry() {
#if defined(HAS_DISPLAY)
  sm().display_.disp_main();
#endif
#if defined(HAS_BUTTON_UI)
  sm().bsl_input_.SetEventCallback(std::bind(
      &AwakeModeIdleState::handle_ui_event, this, std::placeholders::_1));
#elif defined(HAS_TOUCH_UI)
  sm().touch_handler_.SetEventCallback(std::bind(
      &AwakeModeIdleState::handle_ui_event, this, std::placeholders::_1));
#endif
}

void AppSMStates::AwakeModeIdleState::exit() {
  sm().bsl_input_.ClearEventCallback();
}

void AppSMStates::AwakeModeIdleState::loop() {
  if (millis() - sm().last_sensor_publish_ >= AWAKE_SENSOR_INTERVAL) {
#if defined(HAS_TH_SENSOR)
    sm().hw_.read_temp_hmd(sm().device_state_.sensors().temperature,
                           sm().device_state_.sensors().humidity,
                           sm().device_state_.get_use_fahrenheit());
    sm()._publish_sensors();
#endif

#if defined(HAS_BATTERY)
    sm().device_state_.sensors().battery_pct = sm().hw_.read_battery_percent();
    sm().device_state_.sensors().battery_voltage =
        sm().hw_.read_battery_voltage();
    sm()._publish_battery();
#endif
    sm().last_sensor_publish_ = millis();
    sm()._publish_system_state();
#ifdef HOME_BUTTONS_DEBUG
    sm()._log_task_stats();
#endif
  }

#if defined(HAS_DISPLAY)
  if (millis() - sm().last_m_display_redraw_ >= AWAKE_REDRAW_INTERVAL) {
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
#endif

#if defined(HAS_FRONTLIGHT)
  else if (millis() - sm().device_state_.flags().last_user_input_time >
           FRONTLIGHT_TIMEOUT) {
    sm().hw_.set_frontlight(0);
  }
#endif

#if defined(HAS_CHARGER)
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
#if defined(HAS_FRONTLIGHT)
  sm().hw_.set_frontlight(sm().hw_.FL_LED_BRIGHT_DFLT);
#endif
  if (event.final) {
    switch (event.type) {
      case UserInput::EventType::kClickSingle:
      case UserInput::EventType::kClickDouble:
      case UserInput::EventType::kClickTriple:
      case UserInput::EventType::kClickQuad:
        sm()._publish_ui_event(event);
        sm().bsl_input_.LEDBlink(event.btn_id,
                                 UserInput::EventType2NumClicks(event.type), 0,
                                 0, 0, false);
        break;
      case UserInput::EventType::kSwipeDown:
        return transition_to<InfoScreenState>();
      case UserInput::EventType::kSwitchOff:
      case UserInput::EventType::kSwitchOn:
        sm()._publish_ui_event(event);
        break;
      default:
        break;
    }
  } else {
    switch (event.type) {
#if defined(HAS_DISPLAY)
      case UserInput::EventType::kHoldLong2s:
        if (sm().hw_.num_buttons_pressed() == 1) {
          return transition_to<InfoScreenState>();
        }
        break;
#endif
      case UserInput::EventType::kHoldLong5s:
        if (sm().hw_.num_buttons_pressed() == 2) {
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
#if defined(HAS_BUTTON_UI)
  sm().bsl_input_.SetEventCallback(std::bind(
      &SleepModeHandleInput::handle_ui_event, this, std::placeholders::_1));
#elif defined(HAS_TOUCH_UI)
  sm().touch_handler_.SetEventCallback(std::bind(
      &SleepModeHandleInput::handle_ui_event, this, std::placeholders::_1));
#endif
}

void AppSMStates::SleepModeHandleInput::exit() {
  sm().bsl_input_.ClearEventCallback();
}

void AppSMStates::SleepModeHandleInput::loop() {
  if (millis() - sm().input_start_time_ > SLEEP_MODE_INPUT_TIMEOUT) {
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
        sm().bsl_input_.LEDBlink(event.btn_id,
                                 UserInput::EventType2NumClicks(event.type), 0,
                                 0, 0, true);
#if defined(HAS_BATTERY)
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
#if defined(HAS_DISPLAY)
      case UserInput::EventType::kHoldLong2s:
        if (sm().hw_.num_buttons_pressed() == 1) {
          return transition_to<InfoScreenState>();
        }
        break;
      case UserInput::EventType::kHoldLong5s:
        if (sm().hw_.num_buttons_pressed() == 2) {
          return transition_to<SettingsMenuState>();
        }
        break;
#endif
      default:
        break;
    }
  }
}

void AppSMStates::NetConnectingState::entry() {
#if defined(HAS_BUTTON_UI)
  sm().bsl_input_.SetEventCallback(std::bind(
      &NetConnectingState::handle_ui_event, this, std::placeholders::_1));
#elif defined(HAS_TOUCH_UI)
  sm().touch_handler_.SetEventCallback(std::bind(
      &NetConnectingState::handle_ui_event, this, std::placeholders::_1));
#endif
}

void AppSMStates::NetConnectingState::exit() {
  sm().bsl_input_.ClearEventCallback();
}

void AppSMStates::NetConnectingState::loop() {
#if defined(HAS_BUTTON_UI)
  if (sm().network_.get_state() == Network::State::M_CONNECTED) {
    if (sm().user_event_.type != UserInput::EventType::kNone) {
      sm()._publish_ui_event(sm().user_event_);
    }
#if defined(HAS_TH_SENSOR)
    sm().hw_.read_temp_hmd(sm().device_state_.sensors().temperature,
                           sm().device_state_.sensors().humidity,
                           sm().device_state_.get_use_fahrenheit());
    sm()._publish_sensors();
    sm()._publish_system_state();
#endif
#if defined(HAS_BATTERY)
    sm().device_state_.sensors().battery_pct = sm().hw_.read_battery_percent();
    sm()._publish_battery();
#endif
    sm().device_state_.persisted().failed_connections = 0;
    return transition_to<CmdShutdownState>();

  } else if (millis() >= NET_CONNECT_TIMEOUT) {
#if defined(HAS_DISPLAY)
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
#endif
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
#if defined(HAS_DISPLAY)
  sm().info_screen_start_time_ = millis();
  sm().display_.disp_info();
#if defined(HAS_BUTTON_UI)
  sm().bsl_input_.SetEventCallback(std::bind(&InfoScreenState::handle_ui_event,
                                             this, std::placeholders::_1));
#elif defined(HAS_TOUCH_UI)
  sm().device_state_.flags().keep_frontlight_on = true;
  sm().hw_.set_frontlight(sm().hw_.FL_LED_BRIGHT_DFLT);
  sm().touch_handler_.SetEventCallback(std::bind(
      &InfoScreenState::handle_ui_event, this, std::placeholders::_1));
#endif
#endif
}

void AppSMStates::InfoScreenState::exit() {
  sm().bsl_input_.ClearEventCallback();
  sm().device_state_.flags().keep_frontlight_on = false;
}

void AppSMStates::InfoScreenState::loop() {
  if (millis() - sm().info_screen_start_time_ >= INFO_SCREEN_DISP_TIME) {
    sm().debug("info screen timeout");
    if (sm().device_state_.flags().awake_mode) {
      return transition_to<AwakeModeIdleState>();
    } else {
#if defined(HAS_DISPLAY)
      sm().display_.disp_main();
#endif
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
          return transition_to<CmdShutdownState>();
        }
      default:
        break;
    }
  }
}

void AppSMStates::SettingsMenuState::entry() {
  sm().settings_menu_start_time_ = millis();
#if defined(HOME_BUTTONS_INDUSTRIAL)
  sm().bsl_input_.PauseSwitchModeAll();
#endif
#if defined(HAS_DISPLAY)
  sm().display_.disp_settings();
#else
  sm().bsl_input_.LEDPulseAll(0, 2000);
#endif
#if defined(HAS_BUTTON_UI)
  sm().bsl_input_.SetEventCallback(std::bind(
      &SettingsMenuState::handle_ui_event, this, std::placeholders::_1));
#elif defined(HAS_TOUCH_UI)
  sm().device_state_.flags().keep_frontlight_on = true;
  sm().hw_.set_frontlight(sm().hw_.FL_LED_BRIGHT_DFLT);
  sm().touch_handler_.SetEventCallback(std::bind(
      &SettingsMenuState::handle_ui_event, this, std::placeholders::_1));
#endif
}

void AppSMStates::SettingsMenuState::exit() {
  sm().bsl_input_.ClearEventCallback();
  sm().device_state_.flags().keep_frontlight_on = false;
#if !defined(HAS_DISPLAY)
  sm().bsl_input_.LEDOffAll();
#endif
#if defined(HOME_BUTTONS_INDUSTRIAL)
  sm().bsl_input_.ResumeSwitchModeAll();
#endif
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
#if defined(HAS_BUTTON_UI)
  if (event.final) {
    switch (event.type) {
      case UserInput::EventType::kClickSingle:
        switch (event.btn_id) {
          case 1:
            // setup
            sm().device_state_.persisted().restart_to_setup = true;
            sm().device_state_.persisted().silent_restart = true;
            sm().device_state_.save_all();
            sm().info("restarting to setup...");
            ESP.restart();
            break;
          case 2:
            // Wi-Fi setup
            sm().device_state_.persisted().restart_to_wifi_setup = true;
            sm().device_state_.persisted().silent_restart = true;
            sm().device_state_.save_all();
            sm().info("restarting to Wi-Fi setup...");
            ESP.restart();
            break;
          case 3:
            // restart
            sm().info("restarting...");
            ESP.restart();
            break;
          case 4:
            // cancel
            if (sm().device_state_.flags().awake_mode) {
              return transition_to<AwakeModeIdleState>();
            } else {
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
          // factory reset
          return transition_to<FactoryResetState>();
        }
        break;
#if defined(HAS_DISPLAY)
      case UserInput::EventType::kHoldLong2s:
        if (event.btn_id == 1) {
          // device info screen
          return transition_to<DeviceInfoState>();
        }
        break;
#endif
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
#if defined(HAS_DISPLAY)
  sm().display_.disp_device_info();
#endif
#if defined(HAS_BUTTON_UI)
  sm().bsl_input_.SetEventCallback(std::bind(&DeviceInfoState::handle_ui_event,
                                             this, std::placeholders::_1));
#elif defined(HAS_TOUCH_UI)
  sm().device_state_.flags().keep_frontlight_on = true;
  sm().hw_.set_frontlight(sm().hw_.FL_LED_BRIGHT_DFLT);
  sm().touch_handler_.SetEventCallback(std::bind(
      &DeviceInfoState::handle_ui_event, this, std::placeholders::_1));
#endif
}

void AppSMStates::DeviceInfoState::exit() {
  sm().bsl_input_.ClearEventCallback();
  sm().device_state_.flags().keep_frontlight_on = false;
}

void AppSMStates::DeviceInfoState::loop() {
  if (millis() - sm().device_info_start_time_ > DEVICE_INFO_TIMEOUT) {
    sm().debug("device info timeout");
    if (sm().device_state_.flags().awake_mode) {
      return transition_to<AwakeModeIdleState>();
    } else {
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
#if defined(HAS_DISPLAY)
  if (sm().display_.get_ui_state().page != DisplayPage::MAIN) {
    sm().display_.disp_main();
  }
  if (sm().device_state_.persisted().download_mdi_icons) {
    sm()._download_mdi_icons();
    sm().device_state_.persisted().download_mdi_icons = false;
  }
  if (sm().boot_cause_ == BootCause::RESET) {
    sm().display_.disp_main();
  }
#endif
  sm().shutdown_cmd_time_ = millis();
}

void AppSMStates::CmdShutdownState::loop() {
  // wait for timeout
  if (millis() - sm().shutdown_cmd_time_ > SHUTDOWN_DELAY) {
#if defined(HAS_BUTTON_UI)
    sm().bsl_input_.Stop();
#elif defined(HAS_TOUCH_UI)
    sm().touch_handler_.Stop();
#endif
    sm().network_.disconnect();
    return transition_to<NetDisconnectingState>();
  }
}

void AppSMStates::NetDisconnectingState::loop() {
  bool conditions = true;
  conditions =
      conditions && sm().network_.get_state() == Network::State::DISCONNECTED;
#if defined(HAS_DISPLAY)
  conditions = conditions && !sm().display_.busy();
#endif
  if (conditions) {
#if defined(HAS_DISPLAY)
    if (sm().device_state_.flags().display_redraw) {
      sm().device_state_.flags().display_redraw = false;
      sm().display_.disp_main();
    }
    sm().display_.end();
#endif
    return transition_to<ShuttingDownState>();
  }
}

void AppSMStates::ShuttingDownState::loop() {
  bool ended = true;
#if defined(HAS_DISPLAY)
  ended = ended && sm().display_.get_state() == Display::State::IDLE;
#endif
#if defined(HAS_BUTTON_UI)
  ended = ended &&
          sm().bsl_input_.cstate() == ComponentBase::ComponentState::kStopped &&
          !sm().hw_.any_button_pressed();
#endif
#if defined(HAS_TOUCH_UI)
  ended = ended && sm().touch_handler_.cstate() ==
                       ComponentBase::ComponentState::kStopped;
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
#if defined(HAS_SLEEP_MODE)
      sm()._go_to_sleep();
#endif
    }
  }
}

void AppSMStates::FactoryResetState::entry() {
  sm().info("factory reset...");
  sm().network_.disconnect(true);  // erase login data
#if defined(HAS_DISPLAY)
  sm().display_.disp_message("Factory\nRESET...");
  sm().display_.end();
#else
  sm().bsl_input_.LEDBlink(3, 10, 0, 200, 160, false);
#endif
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI)
  sm().bsl_input_.Stop();
#elif defined(HOME_BUTTONS_PRO)
#endif
}

void AppSMStates::FactoryResetState::loop() {
  bool conditions = true;
  conditions =
      conditions && sm().network_.get_state() == Network::State::DISCONNECTED;
#if defined(HAS_DISPLAY)
  conditions = conditions && sm().display_.get_state() == Display::State::IDLE;
#endif
  if (conditions) {
    sm().device_state_.clear_all();
    sm().info("factory reset complete.");
    delay(5000);
    ESP.restart();
  }
}

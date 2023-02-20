#include "app.h"
#include "config.h"
#include "factory.h"
#include "autodiscovery.h"
#include "setup.h"
#include <esp_task_wdt.h>
#include <Arduino.h>

App::App() : m_network(m_device_state),
              m_display(m_device_state)
{
}

void App::setup()
{
  xTaskCreate(_main_task_helper, // Function that should be called
              "MAIN",            // Name of the task (for debugging)
              20000,             // Stack size (bytes)
              this,              // Parameter to pass
              1,                 // Task priority
              &m_main_task_h     // Task handle
  );
  log_d("[DEVICE] main task started.");

  vTaskDelete(NULL);
}

void App::_start_esp_sleep()
{
  esp_sleep_enable_ext1_wakeup(HW.WAKE_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
  if (m_device_state.persisted().wifi_done && m_device_state.persisted().setup_done &&
      !m_device_state.persisted().low_batt_mode && !m_device_state.persisted().check_connection)
  {
    if (m_device_state.persisted().info_screen_showing)
    {
      esp_sleep_enable_timer_wakeup(INFO_SCREEN_DISP_TIME * 1000UL);
    }
    else
    {
      esp_sleep_enable_timer_wakeup(m_device_state.sensor_interval() * 60000000UL);
    }
  }
  log_i("[DEVICE] deep sleep... z z z");
  esp_deep_sleep_start();
}

void App::_go_to_sleep()
{
  m_device_state.save_all();
  HW.set_all_leds(0);
  _start_esp_sleep();
}

std::pair<BootCause, Button *> App::_determine_boot_cause()
{
  BootCause boot_cause = BootCause::RESET;
  int16_t wakeup_pin = 0;
  Button *active_button = nullptr;
  switch (esp_sleep_get_wakeup_cause())
  {
  case ESP_SLEEP_WAKEUP_EXT1:
  {
    uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
    wakeup_pin = (log(GPIO_reason)) / log(2);
    log_i("[DEVICE] wakeup cause: PIN %d", wakeup_pin);
    if (wakeup_pin == HW.BTN1_PIN || wakeup_pin == HW.BTN2_PIN ||
        wakeup_pin == HW.BTN3_PIN || wakeup_pin == HW.BTN4_PIN ||
        wakeup_pin == HW.BTN5_PIN || wakeup_pin == HW.BTN6_PIN)
    {
      boot_cause = BootCause::BUTTON;
    }
    else
    {
      boot_cause = BootCause::RESET;
    }
  }
  break;
  case ESP_SLEEP_WAKEUP_TIMER:
    log_i("[DEVICE] wakeup cause: TIMER");
    boot_cause = BootCause::TIMER;
    break;
  default:
    log_i("[DEVICE] wakeup cause: RESET");
    boot_cause = BootCause::RESET;
    break;
  }
  for (auto &b : m_buttons)
  {
    if (b.get_pin() == wakeup_pin)
    {
      active_button = &b;
    }
  }
  return std::make_pair(boot_cause, active_button);
}

void App::_log_stack_status() const
{
  uint32_t btns_free = uxTaskGetStackHighWaterMark(m_button_task_h);
  uint32_t disp_free = uxTaskGetStackHighWaterMark(m_display_task_h);
  uint32_t net_free = uxTaskGetStackHighWaterMark(m_network_task_h);
  uint32_t leds_free = uxTaskGetStackHighWaterMark(m_leds_task_h);
  uint32_t main_free = uxTaskGetStackHighWaterMark(m_main_task_h);
  uint32_t num_tasks = uxTaskGetNumberOfTasks();
  log_d(
      "[DEVICE] free stack: btns %d, disp %d, net %d, leds %d, main "
      "%d, num tasks %d",
      btns_free, disp_free, net_free, leds_free, main_free,
      num_tasks);
  uint32_t esp_free_heap = ESP.getFreeHeap();
  uint32_t esp_min_free_heap = ESP.getMinFreeHeap();
  uint32_t rtos_free_heap = xPortGetFreeHeapSize();
  log_d("[DEVICE] free heap: esp %d, esp min %d, rtos %d",
        esp_free_heap, esp_min_free_heap, rtos_free_heap);
}

void App::_setup_buttons()
{
  std::pair<uint8_t, uint16_t> button_map[NUM_BUTTONS]  =
  {
                 {HW.BTN1_PIN, 1},
                 {HW.BTN6_PIN, 2},
                 {HW.BTN2_PIN, 3},
                 {HW.BTN5_PIN, 4},
                 {HW.BTN3_PIN, 5},
                 {HW.BTN4_PIN, 6}};
  for (uint i=0; i<NUM_BUTTONS; i++)
  {
    m_buttons[i].setup(button_map[i].first, button_map[i].second);
  }
}


void App::_begin_buttons()
{
  for (auto& b : m_buttons)
  {
    b.begin();
  }
}

void App::_end_buttons()
{
  for (auto &b : m_buttons)
  {
    b.end();
  }
}

void App::_button_task(void *param) {
  App* app = static_cast<App*>(param);
  while (true) {
    for (auto& b : app->m_buttons) {
      b.update();
    }
    delay(20);
  }
}

void App::_leds_task(void *param) {
  App* app = static_cast<App*>(param);
  while (true) {
    app->m_leds.update();
    delay(100);
  }
}

void App::_display_task(void *param) {
  App* app = static_cast<App*>(param);
  while (true) {
    app->m_display.update();
    delay(50);
  }
}

void App::_network_task(void *param) {
  App* app = static_cast<App*>(param);
  while (true) {
    app->m_network.update();
    delay(10);
  }
}



void App::_start_button_task() {
  if (m_button_task_h != nullptr) return;
  log_d("[DEVICE] button task started.");
  xTaskCreate(_button_task,    // Function that should be called
              "BUTTON",       // Name of the task (for debugging)
              2000,           // Stack size (bytes)
              this,           // Parameter to pass
              1,             // Task priority
              &m_button_task_h  // Task handle
  );
}

void App::_start_display_task() {
  if (m_display_task_h != nullptr) return;
  log_d("[DEVICE] m_display task started.");
  xTaskCreate(_display_task,    // Function that should be called
              "DISPLAY",       // Name of the task (for debugging)
              5000,            // Stack size (bytes)
              this,            // Parameter to pass
              1,               // Task priority
              &m_display_task_h  // Task handle
  );
}

void App::_start_network_task() {
  if (m_network_task_h != nullptr) return;
  log_d("[DEVICE] network task started.");
  xTaskCreate(_network_task,    // Function that should be called
              "NETWORK",       // Name of the task (for debugging)
              10000,           // Stack size (bytes)
              this,            // Parameter to pass
              1,              // Task priority
              &m_network_task_h  // Task handle
  );
}

void App::_start_leds_task() {
  if (m_leds_task_h != nullptr) return;
  log_d("[DEVICE] leds task started.");
  xTaskCreate(&_leds_task,    // Function that should be called
              "LEDS",       // Name of the task (for debugging)
              2000,         // Stack size (bytes)
              this,         // Parameter to pass
              1,           // Task priority
              &m_leds_task_h  // Task handle
  );
}

void App::_main_task()
{
  log_i("[DEVICE] woke up.");
  log_i("[DEVICE] SW version: %s", SW_VERSION);
  m_device_state.load_all();

  // ------ factory mode ------
  if (m_device_state.factory().serial_number.length() < 1)
  {
    log_i("[DEVICE] first boot, starting factory mode...");
    factory::factory_mode(m_device_state, m_display);
    m_display.begin();
    m_display.disp_welcome();
    m_display.update();
    m_display.end();
    log_i("[DEVICE] factory settings complete. Going to sleep.");
    _start_esp_sleep();
  }

  // ------ init hardware ------
  log_i("[HW] version: %s", m_device_state.factory().hw_version.c_str());
  HW.init(m_device_state.factory().hw_version);
  m_display.begin(); // must be before ledAttachPin (reserves GPIO37 = SPIDQS)
  HW.begin();
  _setup_buttons();

  // ------ after update handler ------
  if (m_device_state.persisted().last_sw_ver != SW_VERSION)
  {
    if (m_device_state.persisted().last_sw_ver.length() > 0)
    {
      log_i("[DEVICE] firmware updated from %s to %s",
            m_device_state.persisted().last_sw_ver.c_str(), SW_VERSION);
      m_device_state.persisted().last_sw_ver = SW_VERSION;
      m_device_state.persisted().send_discovery_config = true;
      m_device_state.save_all();
      m_display.disp_message(UIState::MessageType("Firmware\nupdated to\n") + SW_VERSION);
      m_display.update();
      ESP.restart();
    }
    else
    { // first boot after factory flash
      m_device_state.persisted().last_sw_ver = SW_VERSION;
    }
  }

  // ------ determine power mode ------
  m_device_state.sensors().battery_present = HW.is_battery_present();
  m_device_state.sensors().dc_connected = HW.is_dc_connected();
  log_i("[DEVICE] batt present: %d, DC connected: %d",
        m_device_state.sensors().battery_present, m_device_state.sensors().dc_connected);

  if (m_device_state.sensors().battery_present)
  {
    float batt_voltage = HW.read_battery_voltage();
    log_i("[DEVICE] batt volts: %f", batt_voltage);
    if (m_device_state.sensors().dc_connected)
    {
      // charging
      if (batt_voltage < HW.CHARGE_HYSTERESIS_VOLT)
      {
        m_device_state.sensors().charging = true;
        HW.enable_charger(true);
        m_device_state.flags().awake_mode = true;
      }
      else
      {
        m_device_state.flags().awake_mode = m_device_state.persisted().user_awake_mode;
      }
      m_device_state.persisted().low_batt_mode = false;
    }
    else
    { // dc_connected == false
      if (m_device_state.persisted().low_batt_mode)
      {
        if (batt_voltage >= HW.BATT_HYSTERESIS_VOLT)
        {
          m_device_state.persisted().low_batt_mode = false;
          m_device_state.save_all();
          log_i("[DEVICE] low batt mode disabled");
          ESP.restart(); // to handle m_display update
        }
        else
        {
          log_i("[DEVICE] in low batt mode...");
          _go_to_sleep();
        }
      }
      else
      { // low_batt_mode == false
        if (batt_voltage < HW.MIN_BATT_VOLT)
        {
          // check again
          delay(1000);
          batt_voltage = HW.read_battery_voltage();
          if (batt_voltage < HW.MIN_BATT_VOLT)
          {
            m_device_state.persisted().low_batt_mode = true;
            log_w("[DEVICE] batt voltage too low, low bat mode enabled");
            m_display.disp_message_large(
                "Turned\nOFF\n\nPlease\nrecharge\nbattery!");
            m_display.update();
            _go_to_sleep();
          }
        }
        else if (batt_voltage <= HW.WARN_BATT_VOLT)
        {
          m_device_state.sensors().battery_low = true;
        }
        m_device_state.flags().awake_mode = false;
      }
    }
  }
  else
  { // battery_present == false
    if (m_device_state.sensors().dc_connected)
    {
      m_device_state.persisted().low_batt_mode = false;
      // choose power mode based on user setting
      m_device_state.flags().awake_mode = m_device_state.persisted().user_awake_mode;
    }
    else
    {
      // should never happen
      _go_to_sleep();
    }
  }
  log_i("[DEVICE] usr awake mode: %d, awake mode: %d",
        m_device_state.persisted().user_awake_mode, m_device_state.flags().awake_mode);

  // ------ read sensors ------
  HW.read_temp_hmd(m_device_state.sensors().temperature, m_device_state.sensors().humidity);
  m_device_state.sensors().battery_pct = HW.read_battery_percent();

  // ------ boot cause ------
  auto &&[boot_cause, active_button] = _determine_boot_cause();


  // ------ handle boot cause ------
  switch (boot_cause)
  {
  case BootCause::RESET:
  {
    if (!m_device_state.persisted().silent_restart)
    {
      m_display.disp_message("RESTART...", 0);
      m_display.update();
    }

    if (m_device_state.persisted().restart_to_wifi_setup)
    {
      m_device_state.clear_persisted_flags();
      log_i("[DEVICE] staring Wi-Fi setup...");
      start_wifi_setup(m_device_state, m_display); // resets ESP when done
    }
    else if (m_device_state.persisted().restart_to_setup)
    {
      m_device_state.clear_persisted_flags();
      log_i("[DEVICE] staring setup...");
      start_setup(m_device_state, m_display); // resets ESP when done
    }
    m_device_state.clear_persisted_flags();
    if (!m_device_state.persisted().wifi_done || !m_device_state.persisted().setup_done)
    {
      m_display.disp_welcome();
      m_display.update();
      m_display.end();
      m_display.update();
      _go_to_sleep();
    }
    else
    {
      m_display.disp_main();
      m_display.update();
    }
    if (m_device_state.flags().awake_mode)
    {
      // proceed with awake mode
      m_sm_state = StateMachineState::AWAIT_NET_CONNECT;
    }
    else
    {
      m_display.end();
      m_display.update();
      _go_to_sleep();
    }
    break;
  }
  case BootCause::TIMER:
  {
    if (m_device_state.flags().awake_mode)
    {
      // proceed with awake mode
      m_sm_state = StateMachineState::AWAIT_NET_CONNECT;
    }
    else
    {
      if (m_device_state.persisted().info_screen_showing)
      {
        m_device_state.persisted().info_screen_showing = false;
        m_display.disp_main();
      }
      if (HW.is_charger_in_standby())
      { // hw <= 2.1 doesn't have awake
        // mode when charging
        if (!m_device_state.persisted().charge_complete_showing)
        {
          m_device_state.persisted().charge_complete_showing = true;
          m_display.disp_message_large("Fully\ncharged!");
        }
      }
      // proceed with sensor publish
      m_sm_state = StateMachineState::AWAIT_NET_CONNECT;
    }
    break;
  }
  case BootCause::BUTTON:
  {
    if (!m_device_state.persisted().wifi_done)
    {
      start_wifi_setup(m_device_state, m_display);
    }
    else if (!m_device_state.persisted().setup_done)
    {
      start_setup(m_device_state, m_display);
    }

    if (!m_device_state.flags().awake_mode)
    {
      if (m_device_state.persisted().info_screen_showing)
      {
        m_device_state.persisted().info_screen_showing = false;
        m_display.disp_main();
        m_display.update();
        m_display.end();
        m_display.update();
        _go_to_sleep();
      }
      else if (m_device_state.persisted().charge_complete_showing)
      {
        m_device_state.persisted().charge_complete_showing = false;
        m_display.disp_main();
        m_display.update();
        m_display.end();
        m_display.update();
        _go_to_sleep();
      }
      else if (m_device_state.persisted().check_connection)
      {
        m_device_state.persisted().check_connection = false;
        m_display.disp_main();
        m_display.update();
        m_display.end();
        m_display.update();
        _go_to_sleep();
      }
      else
      {
        _begin_buttons();
        log_i("Active button : %ul", active_button);
        if (active_button != nullptr)
        {
          active_button->init_press();
          _start_button_task();
          m_leds.begin();
          _start_leds_task();
          m_sm_state = StateMachineState::AWAIT_USER_INPUT_FINISH;
        }
        else
        {
          _go_to_sleep();
        }
      }
    }
    else
    {
      // proceed with awake mode
      m_sm_state = StateMachineState::AWAIT_NET_CONNECT;
    }
    break;
  }
  }

  m_display.init_ui_state(UIState{.page = DisplayPage::MAIN});
  m_network.set_mqtt_callback(std::bind(&App::_mqtt_callback, this, std::placeholders::_1, std::placeholders::_2));
  m_network.set_on_connect(std::bind(&App::_net_on_connect, this));

  if (!m_device_state.flags().awake_mode)
  {
    // ######## SLEEP MODE state machine ########
    esp_task_wdt_init(WDT_TIMEOUT_SLEEP, true);
    esp_task_wdt_add(NULL);
    Button::ButtonAction btn_action = Button::IDLE;
    Button::ButtonAction prev_action = Button::IDLE;
    _start_display_task();
    _start_network_task();
    m_network.connect();
    while (true)
    {
      switch (m_sm_state)
      {
      case StateMachineState::AWAIT_USER_INPUT_FINISH:
      {
        if (active_button == nullptr)
        {
          log_e("[DEVICE] active button = null");
          m_sm_state = StateMachineState::CMD_SHUTDOWN;
        }
        else if (active_button->is_press_finished())
        {
          btn_action = active_button->get_action();
          log_d("[DEVICE] BTN_%d pressed - state %s", active_button->get_id(),
                active_button->get_action_name(btn_action));
          switch (btn_action)
          {
          case Button::SINGLE:
          case Button::DOUBLE:
          case Button::TRIPLE:
          case Button::QUAD:
            btn_action = btn_action;
            m_leds.blink(active_button->get_id(),
                       Button::get_action_multi_count(btn_action), true);
            if (m_device_state.sensors().battery_low)
            {
              m_display.disp_message_large(
                  "Battery\nLOW\n\nPlease\nrecharge\nsoon!", 3000);
            }
            m_sm_state = StateMachineState::AWAIT_NET_CONNECT;
            break;
          case Button::LONG_1:
            // info screen - already m_displayed by transient action handler
            m_device_state.persisted().info_screen_showing = true;
            log_d("[DEVICE] m_displayed info screen");
            m_sm_state = StateMachineState::CMD_SHUTDOWN;
            break;
          case Button::LONG_2:
            m_device_state.persisted().restart_to_setup = true;
            m_device_state.save_all();
            ESP.restart();
            break;
          case Button::LONG_3:
            m_device_state.persisted().restart_to_wifi_setup = true;
            m_device_state.save_all();
            ESP.restart();
            break;
          case Button::LONG_4:
            // factory reset
            m_display.disp_message("Factory\nRESET...");
            m_network.disconnect(true); // erase login data
            m_display.end();
            _end_buttons();
            m_sm_state = StateMachineState::AWAIT_FACTORY_RESET;
            break;
          default:
            m_sm_state = StateMachineState::CMD_SHUTDOWN;
            break;
          }
          for (auto& b : m_buttons)
          {
            b.clear();
          }
        }
        else
        {
          Button::ButtonAction new_action = active_button->get_action();
          if (new_action != prev_action)
          {
            switch (new_action)
            {
            case Button::LONG_1:
              // info screen
              m_display.disp_info();
              break;
            case Button::LONG_2:
              m_display.disp_message(
                  "Release\nfor\nSETUP\n\nKeep\nholding\nfor\nWi-Fi "
                  "SETUP");
              break;
            case Button::LONG_3:
              m_display.disp_message(
                  "Release\nfor\nWi-Fi SETUP\n\nKeep\n"
                  "holding\nfor\nFACTORY\nRESET");
              break;
            case Button::LONG_4:
              m_display.disp_message("Release\nfor\nFACTORY\nRESET");
              break;
            }
            prev_action = new_action;
          }
        }
        break;
      }
      case StateMachineState::AWAIT_NET_CONNECT:
      {
        if (m_network.get_state() == Network::State::M_CONNECTED)
        {
          // net_on_connect();
          if (active_button != nullptr && btn_action != Button::IDLE)
          {
            m_network.publish(
                m_device_state
                    .get_button_topic(active_button->get_id(), btn_action)
                    .c_str(),
                BTN_PRESS_PAYLOAD);
          }
          HW.read_temp_hmd(m_device_state.sensors().temperature, m_device_state.sensors().humidity);
          m_device_state.sensors().battery_pct = HW.read_battery_percent();
          _publish_sensors();
          m_device_state.persisted().failed_connections = 0;
          m_sm_state = StateMachineState::CMD_SHUTDOWN;
        }
        else if (millis() >= NET_CONNECT_TIMEOUT)
        {
          log_d("[DEVICE] network connect timeout.");
          if (boot_cause == BootCause::BUTTON)
          {
            m_display.disp_error("Network\nconnection\nnot\nsuccessful", 3000);
            delay(100);
          }
          else if (boot_cause == BootCause::TIMER)
          {
            m_device_state.persisted().failed_connections++;
            if (m_device_state.persisted().failed_connections >= MAX_FAILED_CONNECTIONS)
            {
              m_device_state.persisted().failed_connections = 0;
              m_device_state.persisted().check_connection = true;
              m_display.disp_error("Check\nconnection!");
              delay(100);
            }
          }
          m_sm_state = StateMachineState::CMD_SHUTDOWN;
        }
        break;
      }
      case StateMachineState::CMD_SHUTDOWN:
      {
        _end_buttons();
        m_leds.end();
        m_network.disconnect();
        m_sm_state = StateMachineState::AWAIT_NET_DISCONNECT;
        break;
      }
      case StateMachineState::AWAIT_NET_DISCONNECT:
      {
        if (m_network.get_state() == Network::State::DISCONNECTED)
        {
          if (m_device_state.flags().display_redraw)
          {
            m_device_state.flags().display_redraw = false;
            m_display.disp_main();
            delay(100);
          }
          m_display.end();
          m_sm_state = StateMachineState::AWAIT_SHUTDOWN;
        }
        break;
      }
      case StateMachineState::AWAIT_SHUTDOWN:
      {
        if (m_display.get_state() == Display::State::IDLE &&
            m_leds.get_state() == LEDs::State::IDLE)
        {
          _log_stack_status();
          if (m_device_state.flags().awake_mode)
          {
            m_device_state.persisted().silent_restart = true;
            m_device_state.save_all();
            ESP.restart();
          }
          else
          {
            _go_to_sleep();
          }
        }
        break;
      }
      case StateMachineState::AWAIT_FACTORY_RESET:
      {
        if (m_network.get_state() == Network::State::DISCONNECTED &&
            m_display.get_state() == Display::State::IDLE)
        {
          m_device_state.clear_all();
          log_i("[DEVICE] factory reset complete.");
          ESP.restart();
        }
        break;
      }
      } // end switch()
      delay(10);
    } // end while()
  }
  else
  {
    // ######## AWAKE MODE state machine ########
    esp_task_wdt_init(WDT_TIMEOUT_AWAKE, true);
    esp_task_wdt_add(NULL);
    Button::ButtonAction prev_action = Button::IDLE;
    uint32_t last_sensor_publish = 0;
    uint32_t last_m_display_redraw = 0;
    uint32_t info_screen_start_time = 0;
    m_device_state.persisted().info_screen_showing = false;
    m_device_state.persisted().check_connection = false;
    m_device_state.persisted().charge_complete_showing = false;
    m_display.disp_main();
    _begin_buttons();
    _start_button_task();
    m_leds.begin();
    _start_leds_task();
    _start_display_task();
    _start_network_task();
    m_network.connect();
    while (true)
    {
      switch (m_sm_state)
      {
      case StateMachineState::AWAIT_NET_CONNECT:
      {
        if (m_network.get_state() == Network::State::M_CONNECTED)
        {
          for (auto& b : m_buttons)
          {
            b.clear();
          }
          HW.read_temp_hmd(m_device_state.sensors().temperature, m_device_state.sensors().humidity);
          m_device_state.sensors().battery_pct = HW.read_battery_percent();
          _publish_sensors();
          m_sm_state = StateMachineState::AWAIT_USER_INPUT_START;
        }
        else if (!HW.is_dc_connected())
        {
          m_device_state.sensors().charging = false;
          m_device_state.persisted().info_screen_showing = false;
          m_display.disp_main();
          delay(100);
          m_sm_state = StateMachineState::CMD_SHUTDOWN;
        }
        break;
      }
      case StateMachineState::AWAIT_USER_INPUT_START:
      {
        for (auto& b : m_buttons)
        {
          if (b.get_action() != Button::IDLE)
          {
            active_button = &b;
            break; // for
          }
        }
        if (active_button != nullptr)
        {
          m_sm_state = StateMachineState::AWAIT_USER_INPUT_FINISH;
        }
        else if (millis() - last_sensor_publish >= AWAKE_SENSOR_INTERVAL)
        {
          HW.read_temp_hmd(m_device_state.sensors().temperature, m_device_state.sensors().humidity);
          m_device_state.sensors().battery_pct = HW.read_battery_percent();
          _publish_sensors();
          last_sensor_publish = millis();
          // log stack status
          _log_stack_status();
        }
        else if (millis() - last_m_display_redraw >= AWAKE_REDRAW_INTERVAL)
        {
          if (m_device_state.flags().display_redraw)
          {
            m_device_state.flags().display_redraw = false;
            if (m_device_state.persisted().info_screen_showing)
            {
              m_display.disp_info();
            }
            else
            {
              m_display.disp_main();
            }
          }
          last_m_display_redraw = millis();
        }
        else if (m_device_state.persisted().info_screen_showing &&
                 millis() - info_screen_start_time >=
                     INFO_SCREEN_DISP_TIME)
        {
          m_display.disp_main();
          m_device_state.persisted().info_screen_showing = false;
        }
        else if (!HW.is_dc_connected())
        {
          _publish_awake_mode_avlb();
          m_device_state.sensors().charging = false;
          m_display.disp_main();
          delay(100);
          m_sm_state = StateMachineState::CMD_SHUTDOWN;
        }
        if (m_device_state.sensors().charging)
        {
          if (HW.is_charger_in_standby())
          {
            m_device_state.sensors().charging = false;
            m_display.disp_main();
            if (!m_device_state.persisted().user_awake_mode)
            {
              delay(100);
              m_sm_state = StateMachineState::CMD_SHUTDOWN;
            }
          }
        }
        else
        {
          if (!m_device_state.persisted().user_awake_mode)
          {
            m_display.disp_main();
            delay(100);
            m_sm_state = StateMachineState::CMD_SHUTDOWN;
          }
        }
        break;
      }
      case StateMachineState::AWAIT_USER_INPUT_FINISH:
      {
        if (active_button == nullptr)
        {
          for (auto& b : m_buttons)
          {
            b.clear();
          }
          m_sm_state = StateMachineState::AWAIT_USER_INPUT_START;
        }
        else if (active_button->is_press_finished())
        {
          auto btn_action = active_button->get_action();
          log_d("[DEVICE] BTN_%d pressed - state %s", active_button->get_id(),
                active_button->get_action_name(btn_action));
          switch (btn_action)
          {
          case Button::SINGLE:
          case Button::DOUBLE:
          case Button::TRIPLE:
          case Button::QUAD:
            if (m_device_state.persisted().info_screen_showing)
            {
              m_display.disp_main();
              m_device_state.persisted().info_screen_showing = false;
              m_sm_state = StateMachineState::AWAIT_USER_INPUT_START;
              break;
            }
            m_leds.blink(active_button->get_id(),
                       Button::get_action_multi_count(btn_action));
            m_network.publish(
                m_device_state
                    .get_button_topic(active_button->get_id(), btn_action)
                    .c_str(),
                BTN_PRESS_PAYLOAD);
            m_sm_state = StateMachineState::AWAIT_USER_INPUT_START;
            break;
          case Button::LONG_1:
            if (m_device_state.persisted().info_screen_showing)
            {
              m_display.disp_main();
              m_device_state.persisted().info_screen_showing = false;
              m_sm_state = StateMachineState::AWAIT_USER_INPUT_START;
              break;
            }
            // info screen - already m_displayed by transient action handler
            m_device_state.persisted().info_screen_showing = true;
            info_screen_start_time = millis();
            log_d("[DEVICE] m_displayed info screen");
            m_sm_state = StateMachineState::AWAIT_USER_INPUT_START;
            break;
          case Button::LONG_2:
            m_device_state.persisted().restart_to_setup = true;
            m_device_state.save_all();
            ESP.restart();
            break;
          case Button::LONG_3:
            m_device_state.persisted().restart_to_wifi_setup = true;
            m_device_state.save_all();
            ESP.restart();
            break;
          case Button::LONG_4:
            // factory reset
            m_display.disp_message("Factory\nRESET...");
            m_network.disconnect(true); // erase login data
            m_display.end();
            _end_buttons();
            m_sm_state = StateMachineState::AWAIT_FACTORY_RESET;
            break;
          default:
            m_sm_state = StateMachineState::AWAIT_USER_INPUT_START;
            break;
          }
          for (auto& b : m_buttons)
          {
            b.clear();
          }
          active_button = nullptr;
        }
        else
        {
          auto new_action = active_button->get_action();
          if (new_action != prev_action)
          {
            switch (new_action)
            {
            case Button::LONG_1:
              // info screen
              m_display.disp_info();
              break;
            case Button::LONG_2:
              m_display.disp_message(
                  "Release\nfor\nSETUP\n\nKeep\nholding\nfor\nWi-Fi "
                  "SETUP");
              break;
            case Button::LONG_3:
              m_display.disp_message(
                  "Release\nfor\nWi-Fi SETUP\n\nKeep\n"
                  "holding\nfor\nFACTORY\nRESET");
              break;
            case Button::LONG_4:
              m_display.disp_message("Release\nfor\nFACTORY\nRESET");
              break;
            }
            prev_action = new_action;
          }
        }
        break;
      }
      case StateMachineState::CMD_SHUTDOWN:
      {
        m_device_state.persisted().info_screen_showing = false;
        _end_buttons();
        m_leds.end();
        m_network.disconnect();
        m_sm_state = StateMachineState::AWAIT_NET_DISCONNECT;
        break;
      }
      case StateMachineState::AWAIT_NET_DISCONNECT:
      {
        if (m_network.get_state() == Network::State::DISCONNECTED)
        {
          if (m_device_state.flags().display_redraw)
          {
            m_device_state.flags().display_redraw = false;
            m_display.disp_main();
            delay(100);
          }
          m_display.end();
          m_sm_state = StateMachineState::AWAIT_SHUTDOWN;
        }
        break;
      }
      case StateMachineState::AWAIT_SHUTDOWN:
      {
        if (m_display.get_state() == Display::State::IDLE &&
            m_leds.get_state() == LEDs::State::IDLE)
        {
          _go_to_sleep();
        }
        break;
      }
      case StateMachineState::AWAIT_FACTORY_RESET:
      {
        if (m_network.get_state() == Network::State::DISCONNECTED &&
            m_display.get_state() == Display::State::IDLE)
        {
          m_device_state.clear_all();
          log_i("[DEVICE] factory reset complete.");
          ESP.restart();
        }
        break;
      }
      } // end switch()
      if (ESP.getMinFreeHeap() < MIN_FREE_HEAP)
      {
        log_w("[DEVICE] free heap low, restarting...");
        _log_stack_status();
        m_device_state.persisted().silent_restart = true;
        m_device_state.save_all();
        ESP.restart();
      }
      esp_task_wdt_reset();
      delay(10);
    } // end while()
  }   // end else
}

void App::_publish_sensors() {
  m_network.publish(m_device_state.topics().t_temperature.c_str(),
                  StaticString<32>("%.2f", m_device_state.sensors().temperature).c_str());
  m_network.publish(m_device_state.topics().t_humidity.c_str(),
                  StaticString<32>("%.2f", m_device_state.sensors().humidity).c_str());
  m_network.publish(m_device_state.topics().t_battery.c_str(),
                  StaticString<32>("%u", m_device_state.sensors().battery_pct).c_str());
}

void App::_publish_awake_mode_avlb() {
  if (HW.is_dc_connected()) {
    m_network.publish(m_device_state.topics().t_awake_mode_avlb.c_str(), "online", true);
  } else {
    m_network.publish(m_device_state.topics().t_awake_mode_avlb.c_str(), "offline", true);
  }
}

void App::_mqtt_callback(const char* topic, const char* payload) {
  if (strcmp(topic, m_device_state.topics().t_sensor_interval_cmd.c_str()) == 0) {
    uint16_t mins = atoi(payload);
    if (mins >= SEN_INTERVAL_MIN && mins <= SEN_INTERVAL_MAX) {
      m_device_state.set_sensor_interval(mins);
      m_device_state.save_all();
      m_network.publish(m_device_state.topics().t_sensor_interval_state.c_str(),
                      StaticString<32>("%u", m_device_state.sensor_interval()).c_str(), true);
      update_discovery_config(m_device_state, m_network);
      log_d("[DEVICE] sensor interval set to %d minutes", mins);
      _publish_sensors();
    }
    m_network.publish(m_device_state.topics().t_sensor_interval_cmd.c_str(), nullptr, true);
    return;
  }

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    if (strcmp(topic, m_device_state.topics().t_btn_label_cmd[i].c_str()) == 0) {
      m_device_state.set_btn_label(i, payload);
      log_d("[DEVICE] button %d label changed to: %s", i + 1,
            m_device_state.get_btn_label(i));
      m_network.publish(m_device_state.topics().t_btn_label_state[i].c_str(),
                      m_device_state.get_btn_label(i), true);
      m_network.publish(m_device_state.topics().t_btn_label_cmd[i].c_str(), nullptr, true);
      m_device_state.flags().display_redraw = true;
      return;
    }
  }

  if (strcmp(topic, m_device_state.topics().t_awake_mode_cmd.c_str()) == 0) {
    if (strcmp(payload,"ON") == 0) {
      m_device_state.persisted().user_awake_mode = true;
      m_device_state.flags().awake_mode = true;
      m_device_state.save_all();
      m_network.publish(m_device_state.topics().t_awake_mode_state.c_str(), "ON", true);
      log_d("[DEVICE] user awake mode set to: ON");
      log_d("[DEVICE] resetting to awake mode...");
    } else if (strcmp(payload, "OFF") == 0) {
      m_device_state.persisted().user_awake_mode = false;
      m_device_state.save_all();
      m_network.publish(m_device_state.topics().t_awake_mode_state.c_str(), "OFF", true);
      log_d("[DEVICE] user awake mode set to: OFF");
    }
    m_network.publish(m_device_state.topics().t_awake_mode_cmd.c_str(), nullptr, true);
    return;
  }
}

void App::_net_on_connect() {
  m_network.subscribe(m_device_state.topics().t_cmd + "#");
  delay(100);
  _publish_awake_mode_avlb();
  m_network.publish(m_device_state.topics().t_sensor_interval_state.c_str(),
                  StaticString<32>("%u", m_device_state.sensor_interval()).c_str(), true);
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    auto t = m_device_state.topics().t_btn_label_state[i];
    m_network.publish(t.c_str(), m_device_state.get_btn_label(i), true);
  }
  m_network.publish(m_device_state.topics().t_awake_mode_state.c_str(),
                (m_device_state.persisted().user_awake_mode) ? "ON" : "OFF", true);

  if (m_device_state.persisted().send_discovery_config) {
    m_device_state.persisted().send_discovery_config = false;
    send_discovery_config(m_device_state, m_network);
  }
}

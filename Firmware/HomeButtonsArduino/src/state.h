#ifndef HOMEBUTTONS_STATE_H
#define HOMEBUTTONS_STATE_H

#include <Preferences.h>
#include "buttons.h"
#include "config.h"

class State {
private:
  struct Factory {
    String serial_number = "";  // len = 8
    String random_id = "";      // len = 6
    String model_name = "";     // 1 < len < 20
    String model_id = "";       // len = 2
    String hw_version = "";     // len = 3
    String unique_id = "";      // len = 20
  } m_factory;

  struct Network {
    String ip = "";
    String ap_ssid = "";
    String ap_password = "";
    struct {
      String server = "";
      int32_t port = 0;
      String user = "";
      String password = "";
      String base_topic = "";
      String discovery_prefix = "";
    } mqtt;
  } m_network;

  struct Personalization {
    String device_name = "";
    String btn_labels[NUM_BUTTONS];
    uint16_t sensor_interval = 0;  // minutes
  } m_personalization;

  struct Topics {
    String t_common = "";
    String t_cmd = "";
    String t_btn_press[NUM_BUTTONS];
    String t_btn_press_double[NUM_BUTTONS];
    String t_btn_press_triple[NUM_BUTTONS];
    String t_btn_press_quad[NUM_BUTTONS];
    String t_temperature = "";
    String t_humidity = "";
    String t_battery = "";
    String t_sensor_interval_cmd = "";
    String t_sensor_interval_state = "";
    String t_btn_label_state[NUM_BUTTONS];
    String t_btn_label_cmd[NUM_BUTTONS];
    String t_awake_mode_state = "";
    String t_awake_mode_cmd = "";
    String t_awake_mode_avlb = "";
  } m_topics;

public:
  const Factory& factory() const { return m_factory; }
  void set_serial_number(const String& str) { m_factory.serial_number = str; }
  void set_random_id(const String& str) { m_factory.random_id = str; }
  void set_model_name(const String& str) { m_factory.model_name = str; }
  void set_model_id(const String& str) { m_factory.model_id = str; }
  void set_hw_version(const String& str) { m_factory.hw_version = str; }
  void set_unique_id(const String& str) { m_factory.unique_id = str; }

  const Network& network() const { return m_network; }
  void set_ip(const String& str) { m_network.ip = str; }
  void set_ap_ssid_and_password(const String& ssid, const String& pwd) { m_network.ap_ssid = ssid; m_network.ap_password = pwd; }
  void set_mqtt_parameters(const String& server, int32_t port, const String& user, const String& password, const String& base_topic, const String& discovery_prefix)
  { m_network.mqtt = {server, port, user, password, base_topic, discovery_prefix}; }

  const String& device_name() const { return m_personalization.device_name; }
  void set_device_name(const String& device_name) { m_personalization.device_name = device_name; }
  uint16_t sensor_interval() const { return m_personalization.sensor_interval; }
  void set_sensor_interval(uint16_t interval_min) { m_personalization.sensor_interval = interval_min; }
  String get_btn_label(uint8_t i);
  void set_btn_label(uint8_t i, String label);

  const Topics& topics() const { return m_topics; }


  // #### PERSISTED VARS ####
  bool low_batt_mode = false;
  bool wifi_done = false;
  bool setup_done = false;
  String last_sw_ver = "";
  bool user_awake_mode = false;

  // persisted flags
  bool wifi_quick_connect = false;
  bool charge_complete_showing = false;
  bool info_screen_showing = false;
  bool check_connection = false;
  uint8_t failed_connections = 0;
  bool restart_to_wifi_setup = false;
  bool restart_to_setup = false;
  bool send_discovery_config = false;
  bool silent_restart = false;


  // flags
  bool display_redraw = false;
  bool awake_mode = false;

  // sensor
  float temperature = 0;
  float humidity = 0;
  uint8_t battery_pct = 0;
  bool charging = false;
  bool dc_connected = false;
  bool battery_present = false;
  bool battery_low = false;

  // #### END ####

  // functions
  void save_factory();
  void load_factory();
  void clear_factory();

  void save_user();
  void load_user();
  void clear_user();

  void save_persisted();
  void load_persisted();
  void clear_persisted();
  void clear_persisted_flags();

  void save_all();
  void load_all();
  void clear_all();

  void set_topics();
  String get_button_topic(uint8_t btn_idx, Button::ButtonAction action);

private:
  Preferences preferences;
};

extern State device_state;

#endif // HOMEBUTTONS_STATE_H

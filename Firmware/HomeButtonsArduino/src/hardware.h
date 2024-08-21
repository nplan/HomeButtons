#ifndef HOMEBUTTONS_HARDWARE_H
#define HOMEBUTTONS_HARDWARE_H

#include <Arduino.h>

#include <semver.hpp>
#include "freertos/semphr.h"

#include "types.h"
#include "config.h"
#include "logger.h"

struct HardwareDefinition : public Logger {
 public:
  HardwareDefinition() : Logger("HW") {}
  HardwareDefinition(const HardwareDefinition &) = delete;
  semver::version version;

  // ------ PIN definitions ------
  uint8_t BTN1_PIN;
  uint8_t BTN2_PIN;
  uint8_t BTN3_PIN;
  uint8_t BTN4_PIN;
  uint8_t BTN5_PIN;
  uint8_t BTN6_PIN;

  bool BTN1_ACTIVE_HIGH;
  bool BTN2_ACTIVE_HIGH;
  bool BTN3_ACTIVE_HIGH;
  bool BTN4_ACTIVE_HIGH;
  bool BTN5_ACTIVE_HIGH;
  bool BTN6_ACTIVE_HIGH;

  uint8_t TOUCH_CLICK_PIN;
  bool TOUCH_CLICK_ACTIVE_HIGH;
  uint8_t TOUCH_INT_PIN;
  uint8_t TOUCH_RST_PIN;

  uint8_t LED1_PIN;
  uint8_t LED2_PIN;
  uint8_t LED3_PIN;
  uint8_t LED4_PIN;
  uint8_t LED5_PIN;
  uint8_t LED6_PIN;

  uint8_t FL_LED_EN_PIN;
  uint8_t FL_LED_PIN;

  uint8_t SDA;
  uint8_t SCL;
  uint8_t SDA_1;
  uint8_t SCL_1;
  uint8_t VBAT_ADC;
  uint8_t CHARGER_STDBY;
  uint8_t BOOST_EN;
  uint8_t DC_IN_DETECT;
  uint8_t CHG_ENABLE;

  uint8_t LIGHT_SEN_ADC;

  uint8_t EINK_CS;
  uint8_t EINK_DC;
  uint8_t EINK_RST;
  uint8_t EINK_BUSY;

  // ------ LED analog parameters ------
  uint8_t LED1_CH;
  uint8_t LED2_CH;
  uint8_t LED3_CH;
  uint8_t LED4_CH;
  uint8_t LED5_CH;
  uint8_t LED6_CH;

  uint8_t FL_LED_CH;
  uint8_t FL_LED_BRIGHT_DFLT;

  uint8_t LED_RES;
  uint16_t LED_FREQ;
  uint8_t LED_BRIGHT_DFLT;
  uint8_t LED_MAX_AMB_BRIGHT;

  // ------ battery reading ------
  float BATT_DIVIDER;
  float BATT_ADC_REF_VOLT;
  float MIN_BATT_VOLT;
  float BATT_HYSTERESIS_VOLT;
  float WARN_BATT_VOLT;
  float BATT_FULL_VOLT;
  float BATT_EMPTY_VOLT;
  float BATT_PRESENT_VOLT;
  float DC_DETECT_VOLT;
  float CHARGE_HYSTERESIS_VOLT;

  // battery SoC linear approximation coefficients (used for lithium cells)
  float BATT_SOC_EST_K;
  float BATT_SOC_EST_N;

  // atan SoC approximation coefficients (used for alkaline cells)
  float BAT_SOC_EST_ATAN_A;
  float BAT_SOC_EST_ATAN_B;
  float BAT_SOC_EST_ATAN_C;
  float BAT_SOC_EST_ATAN_D;

  // ------ wakeup ------
  uint64_t WAKE_BITMASK;

  // ------ functions ------
  bool init();

  void begin();

#if defined(HAS_BUTTON_UI)
  uint8_t map_button_num_sw_to_hw(uint8_t hw_num);
  uint8_t button_pin(uint8_t num);
  bool button_pressed(uint8_t num);
  uint8_t num_buttons_pressed();

  void set_led(uint8_t ch, uint8_t brightness,
               uint16_t fade_time = LED_DEFAULT_FADE_TIME);
  void set_led_num(uint8_t num, uint8_t brightness,
                   uint16_t fade_time = LED_DEFAULT_FADE_TIME);
  void set_all_leds(uint8_t brightness,
                    uint16_t fade_time = LED_DEFAULT_FADE_TIME);
  void blink_led(uint8_t num, uint8_t num_blinks, uint8_t brightness);
#endif

#if defined(HAS_BATTERY)
  float read_battery_voltage();
  uint8_t read_battery_percent();
  bool is_battery_present();
#endif

#if defined(HAS_CHARGER)
  bool is_charger_in_standby();
  bool is_dc_connected();
  void enable_charger(bool enable);
#endif

#if defined(HAS_TOUCH_UI)
  bool touch_click_pressed();
#endif
#if defined(HAS_FRONTLIGHT)
  void set_frontlight(uint8_t brightness);
#endif

  bool any_button_pressed();
  void read_temp_hmd(float &tempe, float &hmd, const bool fahrenheit = false);

  const char *get_serial_number() { return factory_params_.serial_number; }
  const char *get_random_id() { return factory_params_.random_id; }
  const char *get_model_id() { return factory_params_.model_id; }
  const char *get_hw_version() { return factory_params_.hw_version; }
  const char *get_model_name() const { return model_name_; }
  const char *get_unique_id() const { return unique_id_; }

  bool factory_params_ok();

  void set_serial_number(const char *serial_number) {
    memcpy(factory_params_.serial_number, serial_number, 8);
  }
  void set_random_id(const char *random_id) {
    memcpy(factory_params_.random_id, random_id, 6);
  }
  void set_model_id(const char *model_id) {
    memcpy(factory_params_.model_id, model_id, 2);
  }
  void set_hw_version(const char *hw_version) {
    memcpy(factory_params_.hw_version, hw_version, 3);
  }

  void write_factory_params() { _write_efuse(); }

  void load_hw_rev_1_0();
  void load_hw_rev_2_0();
  void load_hw_rev_2_2();
  void load_hw_rev_2_3();
  void load_hw_rev_2_4();
  void load_hw_rev_2_5();

  void load_pro_hw_rev_0_1();

  void load_mini_hw_rev_0_1();
  void load_mini_hw_rev_1_1();

  void load_industrial_hw_rev_1_0();

 private:
  struct {
    // members have length +1 for null terminator
    char serial_number[9];
    char random_id[7];
    char model_id[3];
    char hw_version[4];
  } factory_params_{};

  char model_name_[30] = "";
  char unique_id_[22] = "";

  bool _efuse_burned();
  void _read_efuse();
  void _write_efuse();
  void _nvs_2_efuse();
};

#endif  // HOMEBUTTONS_HARDWARE_H

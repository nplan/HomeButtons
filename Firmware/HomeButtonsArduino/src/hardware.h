#ifndef HOMEBUTTONS_HARDWARE_H
#define HOMEBUTTONS_HARDWARE_H

#include <Arduino.h>

#include <semver.hpp>

#include "types.h"
#include "config.h"
#include "logger.h"

struct HardwareDefinition : public Logger {
 public:
  HardwareDefinition() : Logger("HW") {}
  semver::version version;

  // ------ PIN definitions ------
  uint8_t BTN1_PIN;
  uint8_t BTN2_PIN;
  uint8_t BTN3_PIN;
  uint8_t BTN4_PIN;
  uint8_t BTN5_PIN;
  uint8_t BTN6_PIN;

  uint8_t LED1_PIN;
  uint8_t LED2_PIN;
  uint8_t LED3_PIN;
  uint8_t LED4_PIN;
  uint8_t LED5_PIN;
  uint8_t LED6_PIN;

  uint8_t SDA;
  uint8_t SCL;
  uint8_t VBAT_ADC;
  uint8_t CHARGER_STDBY;
  uint8_t BOOST_EN;
  uint8_t DC_IN_DETECT;
  uint8_t CHG_ENABLE;

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

  uint8_t LED_RES;
  uint16_t LED_FREQ;
  uint8_t LED_BRIGHT_DFLT;

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

  // battery SoC linear approximation coeficients
  float BATT_SOC_EST_K;
  float BATT_SOC_EST_N;

  // ------ wakeup ------
  uint64_t WAKE_BITMASK;

  // ------ functions ------
  void init(const HWVersion &hw_version);

  void begin();

  bool any_button_pressed();

  void set_led(uint8_t ch, uint8_t brightness);

  void set_led_num(uint8_t num, uint8_t brightness);

  void set_all_leds(uint8_t brightness);

  void blink_led(uint8_t num, uint8_t num_blinks,
                 uint8_t brightness = LED_DFLT_BRIGHT);

  float read_battery_voltage();

  uint8_t read_battery_percent();

  void read_temp_hmd(float &tempe, float &hmd);

  bool is_charger_in_standby();

  bool is_dc_connected();

  void enable_charger(bool enable);

  bool is_battery_present();

  void load_hw_rev_1_0();
  void load_hw_rev_2_0();
  void load_hw_rev_2_2();
  void load_hw_rev_2_3();
};

#endif  // HOMEBUTTONS_HARDWARE_H

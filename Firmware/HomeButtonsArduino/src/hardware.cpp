#include "hardware.h"
#include "logger.h"

#include <Wire.h>

#include "Adafruit_SHTC3.h"

// Temperature & humidity sensor
TwoWire shtc3_wire = TwoWire(0);
Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();

void HardwareDefinition::init(const HWVersion &hw_version) {
  if (hw_version == "1.0") {
    load_hw_rev_1_0();
    info("configured for hw version: 1.0");
  } else if (hw_version == "2.0") {
    load_hw_rev_2_0();
    info("configured for hw version: 2.0");
  } else if (hw_version == "2.1") {
    load_hw_rev_2_0();
    info("configured for hw version: 2.1");
  } else if (hw_version == "2.2") {
    load_hw_rev_2_2();
    info("configured for hw version: 2.2");
  } else if (hw_version == "2.3") {
    load_hw_rev_2_3();
    info("configured for hw version: 2.3");
  } else {
    error("HW rev %s not supported", hw_version.c_str());
  }
}

void HardwareDefinition::begin() {
  pinMode(BTN1_PIN, INPUT);
  pinMode(BTN2_PIN, INPUT);
  pinMode(BTN3_PIN, INPUT);
  pinMode(BTN4_PIN, INPUT);
  pinMode(BTN5_PIN, INPUT);
  pinMode(BTN6_PIN, INPUT);

  pinMode(CHARGER_STDBY, INPUT_PULLUP);

  if (version >= semver::version{2, 2, 0}) {
    pinMode(DC_IN_DETECT, INPUT);
    pinMode(CHG_ENABLE, OUTPUT);
  }

  ledcSetup(LED1_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED1_PIN, LED1_CH);

  ledcSetup(LED2_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED2_PIN, LED2_CH);

  ledcSetup(LED3_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED3_PIN, LED3_CH);

  ledcSetup(LED4_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED4_PIN, LED4_CH);

  ledcSetup(LED5_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED5_PIN, LED5_CH);

  ledcSetup(LED6_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED6_PIN, LED6_CH);

  // battery voltage adc
  analogReadResolution(BAT_RES_BITS);
  analogSetPinAttenuation(VBAT_ADC, ADC_11db);
}

bool HardwareDefinition::any_button_pressed() {
  return digitalRead(BTN1_PIN) || digitalRead(BTN2_PIN) ||
         digitalRead(BTN3_PIN) || digitalRead(BTN4_PIN) ||
         digitalRead(BTN5_PIN) || digitalRead(BTN6_PIN);
}

void HardwareDefinition::set_led(uint8_t ch, uint8_t brightness) {
  ledcWrite(ch, brightness);
}

void HardwareDefinition::set_led_num(uint8_t num, uint8_t brightness) {
  uint8_t ch;
  switch (num) {
    case 1:
      ch = LED1_CH;
      break;
    case 2:
      ch = LED6_CH;
      break;
    case 3:
      ch = LED2_CH;
      break;
    case 4:
      ch = LED5_CH;
      break;
    case 5:
      ch = LED3_CH;
      break;
    case 6:
      ch = LED4_CH;
      break;
    default:
      return;
  }
  set_led(ch, brightness);
}

void HardwareDefinition::set_all_leds(uint8_t brightness) {
  set_led(LED1_CH, brightness);
  set_led(LED2_CH, brightness);
  set_led(LED3_CH, brightness);
  set_led(LED4_CH, brightness);
  set_led(LED5_CH, brightness);
  set_led(LED6_CH, brightness);
}

void HardwareDefinition::blink_led(uint8_t led, uint8_t num_blinks,
                                   uint8_t brightness) {
  if (num_blinks < 1)
    num_blinks = 1;
  else if (num_blinks > 4)
    num_blinks = 4;

  uint16_t on, off;
  switch (num_blinks) {
    case 1:
      on = 800;
      off = 0;
      break;
    case 2:
      on = 100;
      off = 300;
      break;
    case 3:
      on = 67;
      off = 200;
      break;
    case 4:
      on = 50;
      off = 150;
      break;
    default:
      on = 50;
      off = 150;
      break;
  }

  for (uint8_t i = 0; i < num_blinks; i++) {
    set_led_num(led, brightness);
    delay(on);
    set_led_num(led, 0);
    delay(off);
  }
}

float HardwareDefinition::read_battery_voltage() {
  return analogRead(VBAT_ADC) / 4095.0 * BATT_ADC_REF_VOLT / BATT_DIVIDER;
}

uint8_t HardwareDefinition::read_battery_percent() {
  if (!is_battery_present()) return 0;
  float pct = 100 * (read_battery_voltage() - BATT_EMPTY_VOLT) /
              (BATT_FULL_VOLT - BATT_EMPTY_VOLT);
  if (pct < 0.0)
    pct = 0;
  else if (pct > 100.0)
    pct = 100;
  return (uint8_t)round(pct);
}

void HardwareDefinition::read_temp_hmd(float &temp, float &hmd) {
  shtc3_wire.begin(
      (int)SDA,
      (int)SCL);  // must be cast to int otherwise wrong begin() is called
  shtc3.begin(&shtc3_wire);
  sensors_event_t humidity_event, temp_event;
  shtc3.getEvent(&humidity_event, &temp_event);
  shtc3.sleep(true);
  temp = temp_event.temperature;
  hmd = humidity_event.relative_humidity;
}

bool HardwareDefinition::is_charger_in_standby() {
  return !digitalRead(CHARGER_STDBY);
}

bool HardwareDefinition::is_dc_connected() {
  if (version >= semver::version{2, 2, 0}) {
    return digitalRead(DC_IN_DETECT);
  } else {
    // hardware hack for powering v2.1 with USB-C
    return HardwareDefinition::read_battery_voltage() >= DC_DETECT_VOLT;
  }
}

void HardwareDefinition::enable_charger(bool enable) {
  if (version >= semver::version{2, 2, 0}) {
    digitalWrite(CHG_ENABLE, enable);
    debug("charger enabled: %d", enable);
  } else {
    debug("this HW version doesn't support charger control.");
  }
}

bool HardwareDefinition::is_battery_present() {
  float volt = read_battery_voltage();
  if (version >= semver::version{2, 2, 0}) {
    return volt >= BATT_PRESENT_VOLT;
  } else {
    // hardware hack for powering v2.1 with USB-C
    return volt >= BATT_PRESENT_VOLT && volt < DC_DETECT_VOLT;
  }
}

void HardwareDefinition::load_hw_rev_1_0() {  // ------ PIN definitions ------
  version = semver::version{1, 0, 0};
  BTN1_PIN = 1;
  BTN2_PIN = 2;
  BTN3_PIN = 3;
  BTN4_PIN = 4;
  BTN5_PIN = 5;
  BTN6_PIN = 6;

  LED1_PIN = 15;
  LED2_PIN = 16;
  LED3_PIN = 17;
  LED4_PIN = 37;
  LED5_PIN = 38;
  LED6_PIN = 45;

  SDA = 10;
  SCL = 11;
  VBAT_ADC = 14;
  CHARGER_STDBY = 12;
  BOOST_EN = 13;
  DC_IN_DETECT = 0;  // Not available
  CHG_ENABLE = 0;    // Not available

  EINK_CS = 5;
  EINK_DC = 8;
  EINK_RST = 9;
  EINK_BUSY = 7;

  // ------ LED analog parameters ------
  LED1_CH = 0;
  LED2_CH = 1;
  LED3_CH = 2;
  LED4_CH = 3;
  LED5_CH = 4;
  LED6_CH = 5;

  LED_RES = 8;
  LED_FREQ = 1000;
  LED_BRIGHT_DFLT = 20;

  // ------ battery reading ------“
  BAT_RES_BITS = 12;
  BATT_DIVIDER = 0.5;
  BATT_ADC_REF_VOLT = 2.6;
  MIN_BATT_VOLT = 3.3;
  BATT_HYSTERESIS_VOLT = 3.5;
  WARN_BATT_VOLT = 3.5;
  BATT_FULL_VOLT = 4.2;
  BATT_EMPTY_VOLT = 3.3;
  BATT_PRESENT_VOLT = 2.7;
  DC_DETECT_VOLT = 4.5;
  CHARGE_HYSTERESIS_VOLT = 4.0;

  // ------ wakeup ------
  WAKE_BITMASK = 0x7E;
}

void HardwareDefinition::load_hw_rev_2_0() {  // ------ PIN definitions ------
  version = semver::version{2, 0, 0};
  BTN1_PIN = 5;
  BTN2_PIN = 6;
  BTN3_PIN = 21;
  BTN4_PIN = 1;
  BTN5_PIN = 3;
  BTN6_PIN = 4;

  LED1_PIN = 15;
  LED2_PIN = 16;
  LED3_PIN = 17;
  LED4_PIN = 2;
  LED5_PIN = 38;
  LED6_PIN = 37;

  SDA = 10;
  SCL = 11;
  VBAT_ADC = 14;
  CHARGER_STDBY = 12;
  BOOST_EN = 13;
  DC_IN_DETECT = 0;  // Not available
  CHG_ENABLE = 0;    // Not available

  EINK_CS = 34;
  EINK_DC = 8;
  EINK_RST = 9;
  EINK_BUSY = 7;

  // ------ LED analog parameters ------
  LED1_CH = 0;
  LED2_CH = 1;
  LED3_CH = 2;
  LED4_CH = 3;
  LED5_CH = 4;
  LED6_CH = 5;

  LED_RES = 8;
  LED_FREQ = 1000;
  LED_BRIGHT_DFLT = 100;

  // ------ battery reading ------“
  BAT_RES_BITS = 12;
  BATT_DIVIDER = 0.5;
  BATT_ADC_REF_VOLT = 2.6;
  MIN_BATT_VOLT = 3.3;
  BATT_HYSTERESIS_VOLT = 3.4;
  WARN_BATT_VOLT = 3.5;
  BATT_FULL_VOLT = 4.2;
  BATT_EMPTY_VOLT = 3.3;
  BATT_PRESENT_VOLT = 2.7;
  DC_DETECT_VOLT = 4.5;
  CHARGE_HYSTERESIS_VOLT = 4.0;

  // ------ wakeup ------
  WAKE_BITMASK = 0x20007A;
}

void HardwareDefinition::load_hw_rev_2_2() {  // ------ PIN definitions ------
  version = semver::version{2, 2, 0};
  BTN1_PIN = 5;
  BTN2_PIN = 6;
  BTN3_PIN = 21;
  BTN4_PIN = 1;
  BTN5_PIN = 3;
  BTN6_PIN = 4;

  LED1_PIN = 15;
  LED2_PIN = 16;
  LED3_PIN = 17;
  LED4_PIN = 2;
  LED5_PIN = 38;
  LED6_PIN = 37;

  SDA = 10;
  SCL = 11;
  VBAT_ADC = 14;
  CHARGER_STDBY = 12;
  BOOST_EN = 13;
  DC_IN_DETECT = 33;
  CHG_ENABLE = 45;

  EINK_CS = 34;
  EINK_DC = 8;
  EINK_RST = 9;
  EINK_BUSY = 7;

  // ------ LED analog parameters ------
  LED1_CH = 0;
  LED2_CH = 1;
  LED3_CH = 2;
  LED4_CH = 3;
  LED5_CH = 4;
  LED6_CH = 5;

  LED_RES = 8;
  LED_FREQ = 1000;
  LED_BRIGHT_DFLT = 100;

  // ------ battery reading ------“
  BAT_RES_BITS = 12;
  BATT_DIVIDER = 0.5;
  BATT_ADC_REF_VOLT = 2.6;
  MIN_BATT_VOLT = 3.3;
  BATT_HYSTERESIS_VOLT = 3.4;
  WARN_BATT_VOLT = 3.5;
  BATT_FULL_VOLT = 4.2;
  BATT_EMPTY_VOLT = 3.3;
  BATT_PRESENT_VOLT = 2.7;
  DC_DETECT_VOLT = 4.5;
  CHARGE_HYSTERESIS_VOLT = 4.0;

  // ------ wakeup ------
  WAKE_BITMASK = 0x20007A;
}

void HardwareDefinition::load_hw_rev_2_3() {  // ------ PIN definitions ------
  version = semver::version{2, 3, 0};
  BTN1_PIN = 5;
  BTN2_PIN = 6;
  BTN3_PIN = 21;
  BTN4_PIN = 1;
  BTN5_PIN = 14;
  BTN6_PIN = 4;

  LED1_PIN = 15;
  LED2_PIN = 16;
  LED3_PIN = 17;
  LED4_PIN = 2;
  LED5_PIN = 38;
  LED6_PIN = 37;

  SDA = 10;
  SCL = 11;
  VBAT_ADC = 3;
  CHARGER_STDBY = 12;
  BOOST_EN = 13;
  DC_IN_DETECT = 33;
  CHG_ENABLE = 45;

  EINK_CS = 34;
  EINK_DC = 8;
  EINK_RST = 9;
  EINK_BUSY = 7;

  // ------ LED analog parameters ------
  LED1_CH = 0;
  LED2_CH = 1;
  LED3_CH = 2;
  LED4_CH = 3;
  LED5_CH = 4;
  LED6_CH = 5;

  LED_RES = 8;
  LED_FREQ = 1000;
  LED_BRIGHT_DFLT = 100;

  // ------ battery reading ------“
  BAT_RES_BITS = 12;
  BATT_DIVIDER = 0.5;
  BATT_ADC_REF_VOLT = 2.6;
  MIN_BATT_VOLT = 3.3;
  BATT_HYSTERESIS_VOLT = 3.4;
  WARN_BATT_VOLT = 3.5;
  BATT_FULL_VOLT = 4.2;
  BATT_EMPTY_VOLT = 3.3;
  BATT_PRESENT_VOLT = 2.7;
  DC_DETECT_VOLT = 4.5;
  CHARGE_HYSTERESIS_VOLT = 4.0;

  // ------ wakeup ------
  WAKE_BITMASK = 0x204072;
}
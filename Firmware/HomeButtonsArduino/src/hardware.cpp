#include "hardware.h"
#include "logger.h"

#include <Wire.h>

#include "Adafruit_SHTC3.h"

// Struct containing current hardware configuration
HardwareDefinition HW;

// Temperature & humidity sensor
TwoWire shtc3_wire = TwoWire(0);
Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();

void HardwareDefinition::init(const Logger &logger,
                              const HWVersion &hw_version) {
  if (hw_version == "1.0") {
    HW = hw_rev_1_0;
    logger.info("configured for hw version: 1.0");
  } else if (hw_version == "2.0") {
    HW = hw_rev_2_0;
    logger.info("configured for hw version: 2.0");
  } else if (hw_version == "2.1") {
    HW = hw_rev_2_0;
    logger.info("configured for hw version: 2.1");
  } else if (hw_version == "2.2") {
    HW = hw_rev_2_2;
    logger.info("configured for hw version: 2.2");
  } else if (hw_version == "2.3") {
    HW = hw_rev_2_3;
    logger.info("configured for hw version: 2.3");
  } else {
    logger.error("HW rev %s not supported", hw_version.c_str());
  }
}

void HardwareDefinition::begin() {
  pinMode(HW.BTN1_PIN, INPUT);
  pinMode(HW.BTN2_PIN, INPUT);
  pinMode(HW.BTN3_PIN, INPUT);
  pinMode(HW.BTN4_PIN, INPUT);
  pinMode(HW.BTN5_PIN, INPUT);
  pinMode(HW.BTN6_PIN, INPUT);

  pinMode(HW.CHARGER_STDBY, INPUT_PULLUP);

  if (HW.version >= semver::version{2, 2, 0}) {
    pinMode(HW.DC_IN_DETECT, INPUT);
    pinMode(HW.CHG_ENABLE, OUTPUT);
  }

  ledcSetup(HW.LED1_CH, HW.LED_FREQ, HW.LED_RES);
  ledcAttachPin(HW.LED1_PIN, HW.LED1_CH);

  ledcSetup(HW.LED2_CH, HW.LED_FREQ, HW.LED_RES);
  ledcAttachPin(HW.LED2_PIN, HW.LED2_CH);

  ledcSetup(HW.LED3_CH, HW.LED_FREQ, HW.LED_RES);
  ledcAttachPin(HW.LED3_PIN, HW.LED3_CH);

  ledcSetup(HW.LED4_CH, HW.LED_FREQ, HW.LED_RES);
  ledcAttachPin(HW.LED4_PIN, HW.LED4_CH);

  ledcSetup(HW.LED5_CH, HW.LED_FREQ, HW.LED_RES);
  ledcAttachPin(HW.LED5_PIN, HW.LED5_CH);

  ledcSetup(HW.LED6_CH, HW.LED_FREQ, HW.LED_RES);
  ledcAttachPin(HW.LED6_PIN, HW.LED6_CH);

  // battery voltage adc
  analogReadResolution(HW.BAT_RES_BITS);
  analogSetPinAttenuation(HW.VBAT_ADC, ADC_11db);
}

bool HardwareDefinition::any_button_pressed() {
  return digitalRead(HW.BTN1_PIN) || digitalRead(HW.BTN2_PIN) ||
         digitalRead(HW.BTN3_PIN) || digitalRead(HW.BTN4_PIN) ||
         digitalRead(HW.BTN5_PIN) || digitalRead(HW.BTN6_PIN);
}

void HardwareDefinition::set_led(uint8_t ch, uint8_t brightness) {
  ledcWrite(ch, brightness);
}

void HardwareDefinition::set_led_num(uint8_t num, uint8_t brightness) {
  uint8_t ch;
  switch (num) {
    case 1:
      ch = HW.LED1_CH;
      break;
    case 2:
      ch = HW.LED6_CH;
      break;
    case 3:
      ch = HW.LED2_CH;
      break;
    case 4:
      ch = HW.LED5_CH;
      break;
    case 5:
      ch = HW.LED3_CH;
      break;
    case 6:
      ch = HW.LED4_CH;
      break;
    default:
      return;
  }
  set_led(ch, brightness);
}

void HardwareDefinition::set_all_leds(uint8_t brightness) {
  set_led(HW.LED1_CH, brightness);
  set_led(HW.LED2_CH, brightness);
  set_led(HW.LED3_CH, brightness);
  set_led(HW.LED4_CH, brightness);
  set_led(HW.LED5_CH, brightness);
  set_led(HW.LED6_CH, brightness);
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
  return analogRead(HW.VBAT_ADC) / 4095.0 * HW.BATT_ADC_REF_VOLT /
         HW.BATT_DIVIDER;
}

uint8_t HardwareDefinition::read_battery_percent() {
  if (!is_battery_present()) return 0;
  float pct = 100 * (read_battery_voltage() - HW.BATT_EMPTY_VOLT) /
              (HW.BATT_FULL_VOLT - HW.BATT_EMPTY_VOLT);
  if (pct < 0.0)
    pct = 0;
  else if (pct > 100.0)
    pct = 100;
  return (uint8_t)round(pct);
}

void HardwareDefinition::read_temp_hmd(float &temp, float &hmd) {
  shtc3_wire.begin(
      (int)HW.SDA,
      (int)HW.SCL);  // must be cast to int otherwise wrong begin() is called
  shtc3.begin(&shtc3_wire);
  sensors_event_t humidity_event, temp_event;
  shtc3.getEvent(&humidity_event, &temp_event);
  shtc3.sleep(true);
  temp = temp_event.temperature;
  hmd = humidity_event.relative_humidity;
}

bool HardwareDefinition::is_charger_in_standby() {
  return !digitalRead(HW.CHARGER_STDBY);
}

bool HardwareDefinition::is_dc_connected() {
  if (HW.version >= semver::version{2, 2, 0}) {
    return digitalRead(HW.DC_IN_DETECT);
  } else {
    // hardware hack for powering v2.1 with USB-C
    return HardwareDefinition::read_battery_voltage() >= HW.DC_DETECT_VOLT;
  }
}

void HardwareDefinition::enable_charger(const Logger &logger, bool enable) {
  if (HW.version >= semver::version{2, 2, 0}) {
    digitalWrite(HW.CHG_ENABLE, enable);
    logger.debug("[HW] charger enabled: %d", enable);
  } else {
    logger.debug("[HW] this HW version doesn't support charger control.");
  }
}

bool HardwareDefinition::is_battery_present() {
  float volt = read_battery_voltage();
  if (HW.version >= semver::version{2, 2, 0}) {
    return volt >= HW.BATT_PRESENT_VOLT;
  } else {
    // hardware hack for powering v2.1 with USB-C
    return volt >= HW.BATT_PRESENT_VOLT && volt < HW.DC_DETECT_VOLT;
  }
}

#include "hardware.h"

// Struct containing current hardware configuration
HardwareDefinition HW;

// Temperature & humidity sensor
TwoWire shtc3_wire = TwoWire(0);
Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();

void init_hardware(String hw_version) {
  if (hw_version == "1.0") {
    HW = hw_rev_1_0;
    log_i("configured for hw version: 1.0");
  } else if (hw_version == "2.0") {
    HW = hw_rev_2_0;
    log_i("configured for hw version: 2.0");
  } else if (hw_version == "2.1") {
    HW = hw_rev_2_0;
    log_i("configured for hw version: 2.1");
  } else if (hw_version == "2.2") {
    HW = hw_rev_2_2;
    log_i("configured for hw version: 2.2");
  } else if (hw_version == "2.3") {
    HW = hw_rev_2_3;
    log_i("configured for hw version: 2.3");
  } else {
    log_e("HW rev %s not supported", hw_version);
  }
}

void begin_hardware() {
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

bool digitalReadAny() {
  return digitalRead(HW.BTN1_PIN) || digitalRead(HW.BTN2_PIN) ||
         digitalRead(HW.BTN3_PIN) || digitalRead(HW.BTN4_PIN) ||
         digitalRead(HW.BTN5_PIN) || digitalRead(HW.BTN6_PIN);
}

void set_led(uint8_t ch, uint8_t brightness) { ledcWrite(ch, brightness); }

void set_all_leds(uint8_t brightness) {
  set_led(HW.LED1_CH, brightness);
  set_led(HW.LED2_CH, brightness);
  set_led(HW.LED3_CH, brightness);
  set_led(HW.LED4_CH, brightness);
  set_led(HW.LED5_CH, brightness);
  set_led(HW.LED6_CH, brightness);
}

float read_battery_voltage() {
  return analogRead(HW.VBAT_ADC) / 4095.0 * HW.BATT_ADC_REF_VOLT /
         HW.BATT_DIVIDER;
}

uint8_t batt_volt2percent(float volt) {
  float pct = 100 * (volt - HW.BATT_EMPTY_VOLT) /
              (HW.BATT_FULL_VOLT - HW.BATT_EMPTY_VOLT);
  if (pct < 0.0)
    pct = 0;
  else if (pct > 100.0)
    pct = 100;
  return (uint8_t)round(pct);
}

void read_temp_hmd(float &temp, float &hmd) {
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

bool is_charger_in_standby() { return !digitalRead(HW.CHARGER_STDBY); }

bool is_dc_connected() {
  if (HW.version >= semver::version{2, 2, 0}) {
    return digitalRead(HW.DC_IN_DETECT);
  } else {
    return false;
  }
}

void enable_charger(bool enable) {
  if (HW.version >= semver::version{2, 2, 0}) {
    digitalWrite(HW.CHG_ENABLE, enable);
  }
}

bool is_battery_present() {
  if (HW.version >= semver::version{2, 2, 0}) {
    return read_battery_voltage() >= HW.BATT_PRESENT_VOLT;
  } else {
    return false;
  }
}

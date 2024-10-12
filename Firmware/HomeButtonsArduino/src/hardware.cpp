#include "hardware.h"
#include "logger.h"
#include "esp_efuse.h"
#include "esp_efuse_custom_table.h"

#include <Wire.h>
#include <Preferences.h>

#include "Adafruit_SHTC3.h"

#include "driver/ledc.h"

#include <math.h>

#if defined(HAS_TH_SENSOR)
// Temperature & humidity sensor
Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();
#endif

// Convert perceived brightness (0-100) to a 12-bit PWM value (0-4095)
uint16_t LED_PCT2PWM(uint8_t perceived_brightness, uint16_t max_pwm_value) {
  // Clamp the input value to the range 0-100
  if (perceived_brightness > 100) perceived_brightness = 100;

  // Normalize the input (0-100) to (0.0-1.0)
  float normalized_brightness = perceived_brightness / 100.0f;

  // Apply gamma correction to account for human perception
  float corrected_brightness = pow(normalized_brightness, LED_GAMMA);

  // Scale the corrected brightness to the custom PWM range (0-max_pwm_value)
  // and round up
  uint16_t pwm_value =
      static_cast<uint16_t>(ceil(corrected_brightness * max_pwm_value));

  return pwm_value;
}

bool HardwareDefinition::init() {
  if (factory_params_ok()) {
    debug("factory params already set");
  } else {
    if (!_efuse_burned()) {
      _nvs_2_efuse();
    }
    _read_efuse();
  }

  if (!factory_params_ok()) {
    error("factory params empty");
    return false;
  }

  // check if SW matches HW
  if (strcmp(get_model_id(), SW_MODEL_ID) != 0) {
    error("SW device model id %s does not match HW device model id %s",
          SW_MODEL_ID, get_model_id());
    return false;
  }

  if (strcmp(get_model_id(), "A1") == 0) {
    strncpy(model_name_, "Home Buttons", sizeof(model_name_));
  } else if (strcmp(get_model_id(), "B1") == 0) {
    strncpy(model_name_, "Home Buttons Mini", sizeof(model_name_));
  } else if (strcmp(get_model_id(), "C1") == 0) {
    strncpy(model_name_, "Home Buttons Pro", sizeof(model_name_));
  } else if (strcmp(get_model_id(), "D1") == 0) {
    strncpy(model_name_, "Home Buttons Industrial", sizeof(model_name_));
  } else {
    error("unknown model id: %s", get_model_id());
    return false;
  }

  snprintf(unique_id_, sizeof(unique_id_), "HBTNS-%s-%s", get_serial_number(),
           get_random_id());

  auto hw_ver = get_hw_version();

#if defined(HOME_BUTTONS_ORIGINAL)
  if (strcmp(hw_ver, "1.0") == 0) {
    load_hw_rev_1_0();
    info("configured for hw version: 1.0");
  } else if (strcmp(hw_ver, "2.0") == 0) {
    load_hw_rev_2_0();
    info("configured for hw version: 2.0");
  } else if (strcmp(hw_ver, "2.1") == 0) {
    load_hw_rev_2_0();
    info("configured for hw version: 2.1");
  } else if (strcmp(hw_ver, "2.2") == 0) {
    load_hw_rev_2_2();
    info("configured for hw version: 2.2");
  } else if (strcmp(hw_ver, "2.3") == 0) {
    load_hw_rev_2_3();
    info("configured for hw version: 2.3");
  } else if (strcmp(hw_ver, "2.4") == 0) {
    load_hw_rev_2_4();
    info("configured for hw version: 2.4");
  } else if (strcmp(hw_ver, "2.5") == 0) {
    load_hw_rev_2_5();
    info("configured for hw version: 2.5");
  } else {
    error("HW rev %s not supported", hw_ver);
    return false;
  }
#elif defined(HOME_BUTTONS_MINI)
  if (strcmp(hw_ver, "0.1") == 0) {
    load_mini_hw_rev_0_1();
    info("configured for hw version: 0.1");
  } else if (strcmp(hw_ver, "1.1") == 0) {
    load_mini_hw_rev_1_1();
    info("configured for hw version: 1.1");
  } else {
    error("HW rev %s not supported", hw_ver);
    return false;
  }
#elif defined(HOME_BUTTONS_PRO)
  if (strcmp(hw_ver, "0.1") == 0) {
    load_pro_hw_rev_0_1();
  } else {
    error("HW rev %s not supported", hw_ver);
    return false;
  }
#elif defined(HOME_BUTTONS_INDUSTRIAL)
  if (strcmp(hw_ver, "1.0") == 0) {
    load_industrial_hw_rev_1_0();
    info("configured for hw version: 1.0");
  } else {
    error("HW rev %s not supported", hw_ver);
    return false;
  }
#endif
  return true;
}

void HardwareDefinition::begin() {
  debug("hw begin");
#if defined(HOME_BUTTONS_ORIGINAL)
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
  pinMode(VBAT_ADC, INPUT);
  analogSetPinAttenuation(VBAT_ADC, ADC_11db);

  // temp sen i2c
  Wire.begin(
      (int)SDA,
      (int)SCL);  // must be cast to int otherwise wrong begin() is called
  shtc3.begin(&Wire);

#elif defined(HOME_BUTTONS_MINI)
  pinMode(BTN1_PIN, INPUT);
  pinMode(BTN2_PIN, INPUT);
  pinMode(BTN3_PIN, INPUT);
  pinMode(BTN4_PIN, INPUT);

  ledcSetup(LED1_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED1_PIN, LED1_CH);

  ledcSetup(LED2_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED2_PIN, LED2_CH);

  ledcSetup(LED3_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED3_PIN, LED3_CH);

  ledcSetup(LED4_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED4_PIN, LED4_CH);

  // battery voltage adc
  pinMode(VBAT_ADC, INPUT);
  analogSetPinAttenuation(VBAT_ADC, ADC_11db);

  // temp sen i2c
  Wire.begin(
      (int)SDA,
      (int)SCL);  // must be cast to int otherwise wrong begin() is called
  shtc3.begin(&Wire);

#elif defined(HOME_BUTTONS_PRO)
  pinMode(TOUCH_CLICK_PIN, INPUT);
  pinMode(TOUCH_INT_PIN, INPUT);
  pinMode(FL_LED_EN_PIN, OUTPUT);
  ledcSetup(FL_LED_CH, LED_FREQ, LED_RES);
  ledcAttachPin(FL_LED_PIN, FL_LED_CH);

  // temp sen i2c
  Wire.begin(
      (int)SDA,
      (int)SCL);  // must be cast to int otherwise wrong begin() is called
  shtc3.begin(&Wire);

  // touch screen
  Wire1.begin(
      (int)SDA_1,
      (int)SCL_1);  // must be cast to int otherwise wrong begin() is called

#elif defined(HOME_BUTTONS_INDUSTRIAL)
  pinMode(BTN1_PIN, INPUT);
  pinMode(BTN2_PIN, INPUT);
  pinMode(BTN3_PIN, INPUT);
  pinMode(BTN4_PIN, INPUT);
  pinMode(BTN5_PIN, INPUT);

  ledcSetup(LED1_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED1_PIN, LED1_CH);

  ledcSetup(LED2_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED2_PIN, LED2_CH);

  ledcSetup(LED3_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED3_PIN, LED3_CH);

  ledcSetup(LED4_CH, LED_FREQ, LED_RES);
  ledcAttachPin(LED4_PIN, LED4_CH);
#endif

  // enable hardware ledc fading
  ledc_fade_func_install(0);
}

#if defined(HAS_BUTTON_UI)

uint8_t HardwareDefinition::map_button_num_sw_to_hw(uint8_t sw_num) {
#if defined(HOME_BUTTONS_ORIGINAL)
  switch (sw_num) {
    case 1:
      return 1;
    case 2:
      return 6;
    case 3:
      return 2;
    case 4:
      return 5;
    case 5:
      return 3;
    case 6:
      return 4;
    default:
      return 0;
  }
#elif defined(HOME_BUTTONS_MINI)
  if (sw_num >= 1 && sw_num <= 4) {
    return sw_num;
  } else {
    return 0;
  }
#elif defined(HOME_BUTTONS_INDUSTRIAL)
  if (sw_num >= 1 && sw_num <= 5) {
    return sw_num;
  } else {
    return 0;
  }
#endif
}

uint8_t HardwareDefinition::button_pin(uint8_t id) {
  uint8_t num = map_button_num_sw_to_hw(id);
  switch (num) {
    case 1:
      return BTN1_PIN;
    case 2:
      return BTN2_PIN;
    case 3:
      return BTN3_PIN;
    case 4:
      return BTN4_PIN;
    case 5:
      return BTN5_PIN;
    case 6:
      return BTN6_PIN;
    default:
      return 0;
  }
}

bool HardwareDefinition::button_pressed(uint8_t id) {
  uint8_t num = map_button_num_sw_to_hw(id);
  switch (num) {
    case 1:
      return digitalRead(BTN1_PIN) == BTN1_ACTIVE_HIGH;
    case 2:
      return digitalRead(BTN2_PIN) == BTN2_ACTIVE_HIGH;
    case 3:
      return digitalRead(BTN3_PIN) == BTN3_ACTIVE_HIGH;
    case 4:
      return digitalRead(BTN4_PIN) == BTN4_ACTIVE_HIGH;
    case 5:
      return digitalRead(BTN5_PIN) == BTN5_ACTIVE_HIGH;
    case 6:
      return digitalRead(BTN6_PIN) == BTN6_ACTIVE_HIGH;
    default:
      return false;
  }
}

uint8_t HardwareDefinition::num_buttons_pressed() {
#if defined(HOME_BUTTONS_ORIGINAL) || defined(HOME_BUTTONS_MINI) || \
    defined(HOME_BUTTONS_PRO)
  uint8_t num = 0;
  for (uint8_t i = 1; i <= NUM_BUTTONS; i++) {
    if (button_pressed(i)) {
      num++;
    }
  }
  return num;
#elif defined(HOME_BUTTONS_INDUSTRIAL)
  uint8_t num = 0;
  for (uint8_t i = 1; i <= 4; i++) {
    if (button_pressed(i)) {
      num++;
    }
  }
  return num;
#endif
}

void HardwareDefinition::set_led(uint8_t ch, uint16_t brightness,
                                 uint16_t fade_time) {
  if (fade_time > 0) {
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, (ledc_channel_t)ch, brightness,
                            fade_time);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, (ledc_channel_t)ch, LEDC_FADE_NO_WAIT);
  } else {
    ledcWrite(ch, brightness);
  }
}

void HardwareDefinition::set_led_num(uint8_t num, uint16_t brightness,
                                     uint16_t fade_time) {
  num = map_button_num_sw_to_hw(num);
  uint8_t ch;

#if defined(HOME_BUTTONS_ORIGINAL)
  switch (num) {
    case 1:
      ch = LED1_CH;
      break;
    case 2:
      ch = LED2_CH;
      break;
    case 3:
      ch = LED3_CH;
      break;
    case 4:
      ch = LED4_CH;
      break;
    case 5:
      ch = LED5_CH;
      break;
    case 6:
      ch = LED6_CH;
      break;
    default:
      return;
  }
#elif defined(HOME_BUTTONS_MINI) || defined(HOME_BUTTONS_INDUSTRIAL)
  switch (num) {
    case 1:
      ch = LED1_CH;
      break;
    case 2:
      ch = LED2_CH;
      break;
    case 3:
      ch = LED3_CH;
      break;
    case 4:
      ch = LED4_CH;
      break;
    default:
      return;
  }
#endif
  set_led(ch, brightness, fade_time);
}

void HardwareDefinition::set_all_leds(uint16_t brightness, uint16_t fade_time) {
#if defined(HOME_BUTTONS_ORIGINAL)
  set_led(LED1_CH, brightness, fade_time);
  set_led(LED2_CH, brightness, fade_time);
  set_led(LED3_CH, brightness, fade_time);
  set_led(LED4_CH, brightness, fade_time);
  set_led(LED5_CH, brightness, fade_time);
  set_led(LED6_CH, brightness, fade_time);
#elif defined(HOME_BUTTONS_MINI) || defined(HOME_BUTTONS_INDUSTRIAL)
  set_led(LED1_CH, brightness, fade_time);
  set_led(LED2_CH, brightness, fade_time);
  set_led(LED3_CH, brightness, fade_time);
  set_led(LED4_CH, brightness, fade_time);
#endif
}

void HardwareDefinition::set_led_pct_num(uint8_t num, uint8_t brightness_pct,
                                         uint16_t fade_time) {
  uint16_t brightness = LED_PCT2PWM(brightness_pct, LED_MAX_PWM);
  set_led_num(num, brightness, fade_time);
}

void HardwareDefinition::set_all_leds_pct(uint8_t brightness_pct,
                                          uint16_t fade_time) {
  uint16_t brightness = LED_PCT2PWM(brightness_pct, LED_MAX_PWM);
  set_all_leds(brightness, fade_time);
}

#endif

#if defined(HAS_BATTERY)
float HardwareDefinition::read_battery_voltage() {
  return (analogReadMilliVolts(VBAT_ADC) / 1000.) / BATT_DIVIDER;
}

uint8_t HardwareDefinition::read_battery_percent() {
#if defined(HOME_BUTTONS_ORIGINAL)
  if (!is_battery_present()) return 0;
  float pct = BATT_SOC_EST_K * read_battery_voltage() + BATT_SOC_EST_N;
  if (pct < 1.0)
    pct = 1;
  else if (pct > 100.0)
    pct = 100;
  return (uint8_t)round(pct);
#elif defined(HOME_BUTTONS_MINI)
  float batvolt = read_battery_voltage();
  float pct = (BAT_SOC_EST_ATAN_A *
                   atan(BAT_SOC_EST_ATAN_B * batvolt + BAT_SOC_EST_ATAN_C) +
               BAT_SOC_EST_ATAN_D) *
              100;
  if (pct < 1.0)
    pct = 1;
  else if (pct > 100.0)
    pct = 100;
  return (uint8_t)round(pct);
#endif
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
#endif

#if defined(HAS_CHARGER)

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
#endif

#if defined(HAS_TOUCH_UI)
bool HardwareDefinition::touch_click_pressed() {
  return digitalRead(TOUCH_CLICK_PIN);
}
#endif

#if defined(HAS_FRONTLIGHT)
void HardwareDefinition::set_frontlight(uint16_t brightness) {
  // 0 - 255
  ledcWrite(FL_LED_CH, brightness);
  if (brightness > 0) {
    digitalWrite(FL_LED_EN_PIN, HIGH);
  } else {
    digitalWrite(FL_LED_EN_PIN, LOW);
  }
}
#endif

bool HardwareDefinition::any_button_pressed() {
  return num_buttons_pressed() > 0;
}

#if defined(HAS_TH_SENSOR)
void HardwareDefinition::read_temp_hmd(float &temp, float &hmd,
                                       const bool fahrenheit) {
  shtc3.reset();
  shtc3.sleep(false);
  sensors_event_t humidity_event, temp_event;
  shtc3.getEvent(&humidity_event, &temp_event);
  shtc3.sleep(true);
  if (fahrenheit) {
    temp = temp_event.temperature * 1.8 + 32;
  } else {
    temp = temp_event.temperature;
  }
  hmd = humidity_event.relative_humidity;
  debug("Sensor read: temp: %.2f C/F, hmd: %.2f %%", temp, hmd);
}
#endif

bool HardwareDefinition::factory_params_ok() {
  return factory_params_.serial_number[0] != 0 &&
         factory_params_.random_id[0] != 0 &&
         factory_params_.model_id[0] != 0 && factory_params_.hw_version[0] != 0;
}

bool HardwareDefinition::_efuse_burned() {
  uint8_t buf[8];
  ESP_ERROR_CHECK(
      esp_efuse_read_field_blob(ESP_EFUSE_USER_DATA_SERIAL_NUMBER, &buf, 8));
  return buf[0] != 0;
}

void HardwareDefinition::_read_efuse() {
  factory_params_ = {};
  ESP_ERROR_CHECK(esp_efuse_read_field_blob(
      ESP_EFUSE_USER_DATA_SERIAL_NUMBER, &factory_params_.serial_number, 64));
  ESP_ERROR_CHECK(esp_efuse_read_field_blob(ESP_EFUSE_USER_DATA_RANDOM_ID,
                                            &factory_params_.random_id, 48));
  ESP_ERROR_CHECK(esp_efuse_read_field_blob(ESP_EFUSE_USER_DATA_MODEL_ID,
                                            &factory_params_.model_id, 16));
  ESP_ERROR_CHECK(esp_efuse_read_field_blob(ESP_EFUSE_USER_DATA_HW_VERSION,
                                            &factory_params_.hw_version, 24));
  info("read efuse factory params: SN=%s, RID=%s, M=%s, HW=%s",
       factory_params_.serial_number, factory_params_.random_id,
       factory_params_.model_id, factory_params_.hw_version);
}

void HardwareDefinition::_write_efuse() {
  ESP_ERROR_CHECK(esp_efuse_batch_write_begin());
  ESP_ERROR_CHECK(esp_efuse_write_field_blob(
      ESP_EFUSE_USER_DATA_SERIAL_NUMBER, &factory_params_.serial_number, 64));
  ESP_ERROR_CHECK(esp_efuse_write_field_blob(ESP_EFUSE_USER_DATA_RANDOM_ID,
                                             &factory_params_.random_id, 48));
  ESP_ERROR_CHECK(esp_efuse_write_field_blob(ESP_EFUSE_USER_DATA_MODEL_ID,
                                             &factory_params_.model_id, 16));
  ESP_ERROR_CHECK(esp_efuse_write_field_blob(ESP_EFUSE_USER_DATA_HW_VERSION,
                                             &factory_params_.hw_version, 24));
  ESP_ERROR_CHECK(esp_efuse_batch_write_commit());
  info("wrote efuse factory params.");
}

void HardwareDefinition::_nvs_2_efuse() {
  info("migrating factory params from nvs to efuse");
  Preferences preferences;
  preferences.begin("factory", true);
  bool success = true;
  success = success && preferences.getString("serial_number",
                                             factory_params_.serial_number, 64);
  success = success &&
            preferences.getString("random_id", factory_params_.random_id, 48);
  success = success &&
            preferences.getString("model_id", factory_params_.model_id, 16);
  success = success &&
            preferences.getString("hw_version", factory_params_.hw_version, 24);
  if (success) {
    debug("read factory params from nvs: SN=%s, RID=%s, M=%s, HW=%s",
          factory_params_.serial_number, factory_params_.random_id,
          factory_params_.model_id, factory_params_.hw_version);
    _write_efuse();
  } else {
    error("failed to read factory params from nvs");
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

  BTN1_ACTIVE_HIGH = true;
  BTN2_ACTIVE_HIGH = true;
  BTN3_ACTIVE_HIGH = true;
  BTN4_ACTIVE_HIGH = true;
  BTN5_ACTIVE_HIGH = true;
  BTN6_ACTIVE_HIGH = true;

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

  LED_RES = 12;
  LED_FREQ = 1000;
  LED_MAX_PWM = 660;

  // ------ battery reading ------“
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
  BATT_SOC_EST_K = 126.58;
  BATT_SOC_EST_N = -425.32;

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

  BTN1_ACTIVE_HIGH = true;
  BTN2_ACTIVE_HIGH = true;
  BTN3_ACTIVE_HIGH = true;
  BTN4_ACTIVE_HIGH = true;
  BTN5_ACTIVE_HIGH = true;
  BTN6_ACTIVE_HIGH = true;

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

  LED_RES = 12;
  LED_FREQ = 1000;
  LED_MAX_PWM = 4000;

  // ------ battery reading ------“
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
  BATT_SOC_EST_K = 126.58;
  BATT_SOC_EST_N = -425.32;

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

  BTN1_ACTIVE_HIGH = true;
  BTN2_ACTIVE_HIGH = true;
  BTN3_ACTIVE_HIGH = true;
  BTN4_ACTIVE_HIGH = true;
  BTN5_ACTIVE_HIGH = true;
  BTN6_ACTIVE_HIGH = true;

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

  LED_RES = 12;
  LED_FREQ = 1000;
  LED_MAX_PWM = 4000;

  // ------ battery reading ------“
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
  BATT_SOC_EST_K = 126.58;
  BATT_SOC_EST_N = -425.32;

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

  BTN1_ACTIVE_HIGH = true;
  BTN2_ACTIVE_HIGH = true;
  BTN3_ACTIVE_HIGH = true;
  BTN4_ACTIVE_HIGH = true;
  BTN5_ACTIVE_HIGH = true;
  BTN6_ACTIVE_HIGH = true;

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

  LED_RES = 12;
  LED_FREQ = 1000;
  LED_MAX_PWM = 4000;

  // ------ battery reading ------“
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
  BATT_SOC_EST_K = 126.58;
  BATT_SOC_EST_N = -425.32;

  // ------ wakeup ------
  WAKE_BITMASK = 0x204072;
}

void HardwareDefinition::load_hw_rev_2_4() {  // ------ PIN definitions ------
  version = semver::version{2, 4, 0};
  BTN1_PIN = 5;
  BTN2_PIN = 6;
  BTN3_PIN = 21;
  BTN4_PIN = 1;
  BTN5_PIN = 14;
  BTN6_PIN = 4;

  BTN1_ACTIVE_HIGH = true;
  BTN2_ACTIVE_HIGH = true;
  BTN3_ACTIVE_HIGH = true;
  BTN4_ACTIVE_HIGH = true;
  BTN5_ACTIVE_HIGH = true;
  BTN6_ACTIVE_HIGH = true;

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

  LED_RES = 12;
  LED_FREQ = 1000;
  LED_MAX_PWM = 750;

  // ------ battery reading ------“
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
  BATT_SOC_EST_K = 126.58;
  BATT_SOC_EST_N = -425.32;

  // ------ wakeup ------
  WAKE_BITMASK = 0x204072;
}

void HardwareDefinition::load_hw_rev_2_5() {  // ------ PIN definitions ------
  version = semver::version{2, 5, 0};
  BTN1_PIN = 5;
  BTN2_PIN = 6;
  BTN3_PIN = 21;
  BTN4_PIN = 1;
  BTN5_PIN = 14;
  BTN6_PIN = 4;

  BTN1_ACTIVE_HIGH = true;
  BTN2_ACTIVE_HIGH = true;
  BTN3_ACTIVE_HIGH = true;
  BTN4_ACTIVE_HIGH = true;
  BTN5_ACTIVE_HIGH = true;
  BTN6_ACTIVE_HIGH = true;

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

  LED_RES = 12;
  LED_FREQ = 1000;
  LED_MAX_PWM = 750;

  // ------ battery reading ------“
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
  BATT_SOC_EST_K = 126.58;
  BATT_SOC_EST_N = -425.32;

  // ------ wakeup ------
  WAKE_BITMASK = 0x204072;
}

void HardwareDefinition::load_pro_hw_rev_0_1() {  // ------ PIN definitions
                                                  // ------
  version = semver::version{0, 1, 0};

  TOUCH_CLICK_PIN = 5;
  TOUCH_INT_PIN = 6;
  TOUCH_RST_PIN = 4;
  TOUCH_CLICK_ACTIVE_HIGH = true;

  FL_LED_EN_PIN = 1;
  FL_LED_PIN = 2;
  FL_LED_CH = 0;
  FL_LED_BRIGHT_DFLT = 800;

  SDA = 10;
  SCL = 11;
  SDA_1 = 13;
  SCL_1 = 14;

  EINK_CS = 34;
  EINK_DC = 8;
  EINK_RST = 9;
  EINK_BUSY = 7;

  LIGHT_SEN_ADC = 3;

  // ------ LED analog parameters ------
  LED_RES = 12;
  LED_FREQ = 1000;

  // ------ wakeup ------
  WAKE_BITMASK = 0x20;
}

void HardwareDefinition::load_mini_hw_rev_0_1() {
  version = semver::version{0, 1, 0};
  BTN1_PIN = 21;
  BTN2_PIN = 1;
  BTN3_PIN = 14;
  BTN4_PIN = 4;

  BTN1_ACTIVE_HIGH = true;
  BTN2_ACTIVE_HIGH = true;
  BTN3_ACTIVE_HIGH = true;
  BTN4_ACTIVE_HIGH = true;

  LED1_PIN = 17;
  LED2_PIN = 2;
  LED3_PIN = 15;
  LED4_PIN = 5;

  SDA = 10;
  SCL = 11;
  VBAT_ADC = 3;

  EINK_CS = 34;
  EINK_DC = 8;
  EINK_RST = 9;
  EINK_BUSY = 7;

  // ------ LED analog parameters ------
  LED1_CH = 0;
  LED2_CH = 1;
  LED3_CH = 2;
  LED4_CH = 3;

  LED_RES = 12;
  LED_FREQ = 1000;
  LED_MAX_PWM = 750;

  // ------ battery reading ------“
  BATT_DIVIDER = 0.6666667;
  BATT_ADC_REF_VOLT = 2.6;
  MIN_BATT_VOLT = 2.15;
  BATT_HYSTERESIS_VOLT = 2.25;
  WARN_BATT_VOLT = 2.25;
  BATT_FULL_VOLT = 3.25;
  BATT_EMPTY_VOLT = 2.15;
  BAT_SOC_EST_ATAN_A = 0.4294;
  BAT_SOC_EST_ATAN_B = 4.8711;
  BAT_SOC_EST_ATAN_C = -12.9462;
  BAT_SOC_EST_ATAN_D = 0.4849;
  // Measured on VARTA Industrial Pro AA Alkaline

  // ------ wakeup ------
  WAKE_BITMASK = 0x204012;
}

void HardwareDefinition::load_mini_hw_rev_1_1() {
  version = semver::version{0, 1, 0};
  BTN1_PIN = 21;
  BTN2_PIN = 1;
  BTN3_PIN = 14;
  BTN4_PIN = 4;

  BTN1_ACTIVE_HIGH = true;
  BTN2_ACTIVE_HIGH = true;
  BTN3_ACTIVE_HIGH = true;
  BTN4_ACTIVE_HIGH = true;

  LED1_PIN = 17;
  LED2_PIN = 2;
  LED3_PIN = 15;
  LED4_PIN = 5;

  SDA = 10;
  SCL = 11;
  VBAT_ADC = 3;

  EINK_CS = 34;
  EINK_DC = 8;
  EINK_RST = 9;
  EINK_BUSY = 7;

  // ------ LED analog parameters ------
  LED1_CH = 0;
  LED2_CH = 1;
  LED3_CH = 2;
  LED4_CH = 3;

  LED_RES = 12;
  LED_FREQ = 1000;
  LED_MAX_PWM = 750;

  // ------ battery reading ------“
  BATT_DIVIDER = 0.6666667;
  BATT_ADC_REF_VOLT = 2.6;
  MIN_BATT_VOLT = 2.15;
  BATT_HYSTERESIS_VOLT = 2.25;
  WARN_BATT_VOLT = 2.25;
  BATT_FULL_VOLT = 3.25;
  BATT_EMPTY_VOLT = 2.15;
  BAT_SOC_EST_ATAN_A = 0.4294;
  BAT_SOC_EST_ATAN_B = 4.8711;
  BAT_SOC_EST_ATAN_C = -12.9462;
  BAT_SOC_EST_ATAN_D = 0.4849;
  // Measured on VARTA Industrial Pro AA Alkaline

  // ------ wakeup ------
  WAKE_BITMASK = 0x204012;
}

void HardwareDefinition::load_industrial_hw_rev_1_0() {
  version = semver::version{0, 1, 0};
  BTN1_PIN = 6;
  BTN2_PIN = 5;
  BTN3_PIN = 1;
  BTN4_PIN = 21;
  BTN5_PIN = 14;

  BTN1_ACTIVE_HIGH = true;
  BTN2_ACTIVE_HIGH = true;
  BTN3_ACTIVE_HIGH = true;
  BTN4_ACTIVE_HIGH = true;
  BTN5_ACTIVE_HIGH = false;

  LED1_PIN = 17;
  LED2_PIN = 16;
  LED3_PIN = 37;
  LED4_PIN = 2;

  // ------ LED analog parameters ------
  LED1_CH = 0;
  LED2_CH = 1;
  LED3_CH = 2;
  LED4_CH = 3;

  LED_RES = 12;
  LED_FREQ = 1000;
  LED_MAX_PWM = 4095;
}
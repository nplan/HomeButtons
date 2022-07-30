#ifndef HARDWARE_H
#define HARDWARE_H

struct HardwareDefinition {
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
    uint8_t BAT_RES_BITS;
    float BATT_DIVIDER;
    float BATT_ADC_REF_VOLT;
    float MIN_BATT_VOLT;
    float BATT_HISTERESIS_VOLT;
    float WARN_BATT_VOLT;
    float BATT_FULL_VOLT;
    float BATT_EMPTY_VOLT;

    // ------ wakeup ------
    uint64_t WAKE_BITMASK;
};

const HardwareDefinition hw_rev_1_0 {
    // ------ PIN definitions ------
    .BTN1_PIN = 1,
    .BTN2_PIN = 2,
    .BTN3_PIN = 3,
    .BTN4_PIN = 4,
    .BTN5_PIN = 5,
    .BTN6_PIN = 6,

    .LED1_PIN = 15, 
    .LED2_PIN = 16,
    .LED3_PIN = 17,
    .LED4_PIN = 37,
    .LED5_PIN = 38,
    .LED6_PIN = 45,

    .SDA = 10,
    .SCL = 11,
    .VBAT_ADC = 14,
    .CHARGER_STDBY = 12,
    .BOOST_EN = 13,

    .EINK_CS = 5,
    .EINK_DC = 8,
    .EINK_RST = 9,
    .EINK_BUSY = 7,

    // ------ LED analog parameters ------
    .LED1_CH = 0,
    .LED2_CH = 1,
    .LED3_CH = 2,
    .LED4_CH = 3,
    .LED5_CH = 4,
    .LED6_CH = 5,

    .LED_RES = 8,
    .LED_FREQ = 1000,
    .LED_BRIGHT_DFLT = 20,

    // ------ battery reading ------“
    .BAT_RES_BITS = 12,
    .BATT_DIVIDER = 0.5,
    .BATT_ADC_REF_VOLT = 2.6,
    .MIN_BATT_VOLT = 3.3,
    .BATT_HISTERESIS_VOLT = 3.5,
    .WARN_BATT_VOLT = 3.5,
    .BATT_FULL_VOLT = 4.2,
    .BATT_EMPTY_VOLT = 3.3,

    // ------ wakeup ------
    .WAKE_BITMASK = 0x7E
};

const HardwareDefinition hw_rev_2_0 {
    // ------ PIN definitions ------
    .BTN1_PIN = 5,
    .BTN2_PIN = 6,
    .BTN3_PIN = 21,
    .BTN4_PIN = 1,
    .BTN5_PIN = 3,
    .BTN6_PIN = 4,

    .LED1_PIN = 15, 
    .LED2_PIN = 16,
    .LED3_PIN = 17,
    .LED4_PIN = 2,
    .LED5_PIN = 38,
    .LED6_PIN = 37,

    .SDA = 10,
    .SCL = 11,
    .VBAT_ADC = 14,
    .CHARGER_STDBY = 12,
    .BOOST_EN = 13,

    .EINK_CS = 34,
    .EINK_DC = 8,
    .EINK_RST = 9,
    .EINK_BUSY = 7,

    // ------ LED analog parameters ------
    .LED1_CH = 0,
    .LED2_CH = 1,
    .LED3_CH = 2,
    .LED4_CH = 3,
    .LED5_CH = 4,
    .LED6_CH = 5,

    .LED_RES = 8,
    .LED_FREQ = 1000,
    .LED_BRIGHT_DFLT = 100,

    // ------ battery reading ------“
    .BAT_RES_BITS = 12,
    .BATT_DIVIDER = 0.5,
    .BATT_ADC_REF_VOLT = 2.6,
    .MIN_BATT_VOLT = 3.3,
    .BATT_HISTERESIS_VOLT = 3.5,
    .WARN_BATT_VOLT = 3.5,
    .BATT_FULL_VOLT = 4.2,
    .BATT_EMPTY_VOLT = 3.3,

        // ------ wakeup ------
    .WAKE_BITMASK = 0x20007A
};

// Struct containing current hardware configuration
HardwareDefinition HW;

void setup_hardware(uint16_t hw_version) {
    if (hw_version >= 20) {
        HW = hw_rev_2_0;
    }
    else if (hw_version >= 10) {
        HW = hw_rev_1_0;
    }

    pinMode(HW.BTN1_PIN, INPUT);
    pinMode(HW.BTN2_PIN, INPUT);
    pinMode(HW.BTN3_PIN, INPUT);
    pinMode(HW.BTN4_PIN, INPUT);
    pinMode(HW.BTN5_PIN, INPUT);
    pinMode(HW.BTN6_PIN, INPUT);

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

    // ------ delay ------
    delay(100); // wait for peripherals to boot up
}

#endif
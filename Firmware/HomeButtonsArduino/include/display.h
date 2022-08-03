#ifndef DISPLAY_H
#define DISPLAY_H

#include "hardware.h"

#include <qrcode.h>

#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

namespace eink
{

  void begin();

  void hibernate();

  void display_string(const char* string);

  void display_error(const char* string);

  void display_buttons(const char* button_1_text,
                        const char* button_2_text,
                        const char* button_3_text,
                        const char* button_4_text,
                        const char* button_5_text,
                        const char* button_6_text);

  void display_ap_config_screen(String ssid, String password);

  void display_web_config_screen(String ip);

  void display_wifi_connected_screen();

  void display_wifi_not_connected_screen();

  void display_setup_complete_screen();

  void display_please_recharge_soon_screen();

  void display_turned_off_please_recharge_screen();

  void display_please_complete_setup_screen(const char * uid);

  void display_unboxing_screen(const char * uid);

  void display_info_screen(float temp, float humid, uint8_t batt);

  void display_test_screen();

  void display_white_screen();

  void display_black_screen();
} //namespace

#endif
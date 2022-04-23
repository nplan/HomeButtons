#include <GxEPD2_BW.h> // including both doesn't use more code or ram
#include "GxEPD2_display_selection_new_style.h"

#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

#include <qrcode.h>

namespace eink
{

  const int WIDTH = 128;
  const int HEIGHT = 296;

  const uint16_t W = WIDTH / 2;

  const uint16_t H1 = HEIGHT/8;
  const uint16_t H2 = HEIGHT/8 + HEIGHT/4;
  const uint16_t H3 = HEIGHT/8 + 2*HEIGHT/4;
  const uint16_t H4 = HEIGHT/8 + 3*HEIGHT/4;

  const uint16_t num_fonts = 4;
  const GFXfont* fonts[] = {&FreeSansBold24pt7b, &FreeSansBold18pt7b, &FreeSansBold12pt7b, &FreeSansBold9pt7b};
  const uint16_t num_buttons = 4;
  const uint16_t heights[] = {H1, H2, H3, H4};

  void hibernate() {
    display.hibernate();
  }

  void display_string(const char* string) {
    display.setRotation(0);
    display.setTextColor(GxEPD_BLACK);
    display.setTextWrap(true);
    display.setFont(&FreeSans9pt7b);
    display.setPartialWindow(0, 0, display.width(), display.height());

    display.firstPage();
    do {
      display.setCursor(0, 30);
      display.fillScreen(GxEPD_WHITE);
      display.print(string);
    }
    while (display.nextPage());
  }

  void display_error(const char* string) {
    display.setRotation(0);
    display.setTextColor(GxEPD_BLACK);
    display.setTextWrap(false);
    display.setFont(&FreeSansBold9pt7b);
    display.setPartialWindow(0, 0, display.width(), display.height());

    display.firstPage();
    do {
      display.setCursor(0, 0);
      int16_t x, y;
      uint16_t w, h;
      display.getTextBounds("ERROR", 0, 0, &x, &y, &w, &h);
      display.setCursor(WIDTH/2 - w/2, 20);
      display.print("ERROR");

      display.setFont(&FreeSans9pt7b);
      display.setTextWrap(true);
      display.setCursor(0, 60);
      display.print(string);
    }
    while (display.nextPage());
  }

  void display_buttons(const char* button_1_text,
                    const char* button_2_text,
                    const char* button_3_text,
                    const char* button_4_text) {

    const char* texts[] = {button_1_text, button_2_text, button_3_text, button_4_text};

    display.setRotation(0);
    display.setTextColor(GxEPD_BLACK);
    display.setTextWrap(false);
    display.setFullWindow();
    
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);
      
      display.drawFastHLine(0, HEIGHT/4, WIDTH, GxEPD_BLACK);
      display.drawFastHLine(0, 2*HEIGHT/4, WIDTH, GxEPD_BLACK);
      display.drawFastHLine(0, 3*HEIGHT/4, WIDTH, GxEPD_BLACK);
    
      int16_t x, y;
      uint16_t w, h;
      
      // Loop through buttons
      for (uint16_t i=0; i<num_buttons; i++) {
        // Select smaller font if text width greater than display width
        for (uint16_t j=0; j<num_fonts; j++) {
          display.setFont(fonts[j]);
          display.getTextBounds(texts[i], 0, 0, &x, &y, &w, &h);
          if (w <= WIDTH) break;
        }
        display.setCursor(W - w/2, heights[i]+h/2);
        display.print(texts[i]);
      }
    }
    while (display.nextPage());
  }

  void display_ap_config_screen(String ssid, String password) {
    String contents = String("WIFI:T:WPA;S:") + ssid + ";P:" + password + ";;";

    uint8_t version = 6; // 41x41px
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(version)];
    qrcode_initText(&qrcode, qrcodeData, version, ECC_HIGH, contents.c_str());

    display.setRotation(0);
    display.setTextColor(GxEPD_BLACK);
    display.setTextWrap(false);
    display.setFont(&FreeSans9pt7b);

    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);

      int16_t x, y;
      uint16_t w, h;
      display.getTextBounds("Scan:", 0, 0, &x, &y, &w, &h);
      display.setCursor(WIDTH/2 - w/2, 20);
      display.print("Scan:");

      uint16_t qr_x = 23;
      uint16_t qr_y = 35;
      for (uint8_t y = 0; y < qrcode.size; y++) {
        // Each horizontal module
        for (uint8_t x = 0; x < qrcode.size; x++) {

          // Display each module
          if (qrcode_getModule(&qrcode, x, y)) {
            display.drawRect(qr_x+x*2, qr_y+y*2, 2, 2, GxEPD_BLACK);
          }
        }
      }
    display.setFont(&FreeSansBold9pt7b);
    display.getTextBounds("------ or ------", 0, 0, &x, &y, &w, &h);
    display.setCursor(WIDTH/2 - w/2, 153);
    display.print("------ or ------");

    display.setFont(&FreeSans9pt7b);
    display.getTextBounds("Connect to:", 0, 0, &x, &y, &w, &h);
    display.setCursor(WIDTH/2 - w/2, 190);
    display.print("Connect to:");

    display.setCursor(0, 220);
    display.print("WiFi:");
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(0, 235);
    display.print(ssid.c_str());

    display.setFont(&FreeSans9pt7b);
    display.setCursor(0, 260);
    display.print("Password:");
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(0, 275);
    display.print(password.c_str());

    }
    while (display.nextPage());
  }

  void display_web_config_screen(String ip) {
    String contents = String("http://") + ip; 

    uint8_t version = 6; // 41x41px
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(version)];
    qrcode_initText(&qrcode, qrcodeData, version, ECC_HIGH, contents.c_str());

    display.setRotation(0);
    display.setTextColor(GxEPD_BLACK);
    display.setTextWrap(false);
    display.setFont(&FreeSans9pt7b);

    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);

      int16_t x, y;
      uint16_t w, h;
      display.getTextBounds("Scan:", 0, 0, &x, &y, &w, &h);
      display.setCursor(WIDTH/2 - w/2, 20);
      display.print("Scan:");

      uint16_t qr_x = 23;
      uint16_t qr_y = 35;
      for (uint8_t y = 0; y < qrcode.size; y++) {
        // Each horizontal module
        for (uint8_t x = 0; x < qrcode.size; x++) {

          // Display each module
          if (qrcode_getModule(&qrcode, x, y)) {
            display.drawRect(qr_x+x*2, qr_y+y*2, 2, 2, GxEPD_BLACK);
          }
        }
      }
    display.setFont(&FreeSansBold9pt7b);
    display.getTextBounds("------ or ------", 0, 0, &x, &y, &w, &h);
    display.setCursor(WIDTH/2 - w/2, 153);
    display.print("------ or ------");

    display.setFont(&FreeSans9pt7b);
    display.getTextBounds("Go to:", 0, 0, &x, &y, &w, &h);
    display.setCursor(WIDTH/2 - w/2, 200);
    display.print("Go to:");

    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(0, 240);
    display.print("http://");
    display.setCursor(0, 260);
    display.print(ip.c_str());

    }
    while (display.nextPage());
  }

  void display_wifi_connected_screen() {
    display.setRotation(0);
    display.setTextColor(GxEPD_BLACK);
    display.setTextWrap(true);
    display.setFont(&FreeSansBold9pt7b);
    display.setPartialWindow(0, 0, display.width(), display.height());

    display.firstPage();
    do {
      display.setCursor(0, 0);
      display.fillScreen(GxEPD_WHITE);

      int16_t x, y;
      uint16_t w, h;
      display.getTextBounds("WIFI", 0, 0, &x, &y, &w, &h);
      display.setCursor(WIDTH/2 - w/2, 120);
      display.print("WIFI");

      display.getTextBounds("CONNECTED", 0, 0, &x, &y, &w, &h);
      display.setCursor(WIDTH/2 - w/2, 150);
      display.print("CONNECTED");

      display.getTextBounds(":)", 0, 0, &x, &y, &w, &h);
      display.setCursor(WIDTH/2 - w/2, 180);
      display.print(":)");
    }
    while (display.nextPage());
  }

  void display_wifi_not_connected_screen() {
    display.setRotation(0);
    display.setTextColor(GxEPD_BLACK);
    display.setTextWrap(true);
    display.setFont(&FreeSansBold9pt7b);
    display.setPartialWindow(0, 0, display.width(), display.height());

    display.firstPage();
    do {
      display.setCursor(0, 0);
      display.fillScreen(GxEPD_WHITE);

      int16_t x, y;
      uint16_t w, h;
      display.getTextBounds("WIFI", 0, 0, &x, &y, &w, &h);
      display.setCursor(WIDTH/2 - w/2, 90);
      display.print("WIFI");

      display.getTextBounds("NOT", 0, 0, &x, &y, &w, &h);
      display.setCursor(WIDTH/2 - w/2, 120);
      display.print("NOT");

      display.getTextBounds("CONNECTED", 0, 0, &x, &y, &w, &h);
      display.setCursor(WIDTH/2 - w/2, 150);
      display.print("CONNECTED");
    }
    while (display.nextPage());
  }

  void display_setup_complete_screen() {
    display.setRotation(0);
    display.setTextColor(GxEPD_BLACK);
    display.setTextWrap(true);
    display.setFont(&FreeSansBold9pt7b);
    display.setPartialWindow(0, 0, display.width(), display.height());

    display.firstPage();
    do {
      display.setCursor(0, 0);
      display.fillScreen(GxEPD_WHITE);

      int16_t x, y;
      uint16_t w, h;
      display.getTextBounds("SETUP", 0, 0, &x, &y, &w, &h);
      display.setCursor(WIDTH/2 - w/2, 120);
      display.print("SETUP");

      display.getTextBounds("COMPLETE", 0, 0, &x, &y, &w, &h);
      display.setCursor(WIDTH/2 - w/2, 150);
      display.print("COMPLETE");

      display.getTextBounds(":)", 0, 0, &x, &y, &w, &h);
      display.setCursor(WIDTH/2 - w/2, 180);
      display.print(":)");
    }
    while (display.nextPage());
  }

  void display_setup_required_screen() {
    display.setRotation(0);
    display.setTextColor(GxEPD_BLACK);
    display.setTextWrap(true);
    display.setFont(&FreeSansBold9pt7b);
    display.setPartialWindow(0, 0, display.width(), display.height());

    display.firstPage();
    do {
      display.setCursor(0, 0);
      display.fillScreen(GxEPD_WHITE);

      int16_t x, y;
      uint16_t w, h;
      display.getTextBounds("SETUP", 0, 0, &x, &y, &w, &h);
      display.setCursor(WIDTH/2 - w/2, 120);
      display.print("SETUP");

      display.getTextBounds("REQUIRED", 0, 0, &x, &y, &w, &h);
      display.setCursor(WIDTH/2 - w/2, 150);
      display.print("REQUIRED");
    }
    while (display.nextPage());
  }

  void display_dots_screen() {
    display.setRotation(0);
    display.setTextColor(GxEPD_BLACK);
    display.setTextWrap(true);
    display.setFont(&FreeSansBold12pt7b);
    display.setPartialWindow(0, 0, display.width(), display.height());

    display.firstPage();
    do {
      display.setCursor(0, 0);
      display.fillScreen(GxEPD_WHITE);

      int16_t x, y;
      uint16_t w, h;
      display.getTextBounds("...", 0, 0, &x, &y, &w, &h);
      display.setCursor(WIDTH/2 - w/2, HEIGHT/2 - h/2);
      display.print("...");
    }
    while (display.nextPage());
  }

    void display_ok_screen() {
    display.setRotation(0);
    display.setTextColor(GxEPD_BLACK);
    display.setTextWrap(true);
    display.setFont(&FreeSansBold12pt7b);
    display.setPartialWindow(0, 0, display.width(), display.height());

    display.firstPage();
    do {
      display.setCursor(0, 0);
      display.fillScreen(GxEPD_WHITE);

      int16_t x, y;
      uint16_t w, h;
      display.getTextBounds("OK", 0, 0, &x, &y, &w, &h);
      display.setCursor(WIDTH/2 - w/2, HEIGHT/2 - h/2);
      display.print("OK");
    }
    while (display.nextPage());
  }

} //namespace
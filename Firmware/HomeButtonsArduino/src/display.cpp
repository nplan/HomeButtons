#include "display.h"

#include <GxEPD2_BW.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <qrcode.h>

#include "bitmaps.h"
#include "config.h"
#include "hardware.h"

const int WIDTH = 128;
const int HEIGHT = 296;

#define GxEPD2_DISPLAY_CLASS GxEPD2_BW
#define GxEPD2_DRIVER_CLASS GxEPD2_290_T94_V2
#define MAX_DISPLAY_BUFFER_SIZE 65536ul  // e.g.
#define MAX_HEIGHT(EPD)                                      \
  (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) \
       ? EPD::HEIGHT                                         \
       : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))

static GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS,
                            MAX_HEIGHT(GxEPD2_DRIVER_CLASS)>* disp;

static U8G2_FOR_ADAFRUIT_GFX u8g2;

Display display = {};

void Display::begin() {
  if (state != State::IDLE) return;
  disp = new GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS,
                                     MAX_HEIGHT(GxEPD2_DRIVER_CLASS)>(
      GxEPD2_DRIVER_CLASS(/*CS=*/HW.EINK_CS, /*DC=*/HW.EINK_DC,
                          /*RST=*/HW.EINK_RST, /*BUSY=*/HW.EINK_BUSY));
  disp->init();
  u8g2.begin(*disp);
  current_ui_state = {};
  cmd_ui_state = {};
  draw_ui_state = {};
  pre_disappear_ui_state = {};
  state = State::ACTIVE;
  log_i("[DISP] begin");
}

void Display::end() {
  if (state != State::ACTIVE) return;
  state = State::CMD_END;
  log_d("[DISP] cmd end");
}

void Display::update() {
  if (state == State::IDLE) return;

  if (state == State::CMD_END) {
    state = State::ENDING;
    if (current_ui_state.disappearing) {
      draw_ui_state = pre_disappear_ui_state;
    } else {
      disp->hibernate();
      state = State::IDLE;
      log_i("[DISP] ended.");
      return;
    }
  } else if (current_ui_state.disappearing) {
    if (millis() - current_ui_state.appear_time >=
        current_ui_state.disappear_timeout) {
      if (new_ui_cmd) {
        draw_ui_state = cmd_ui_state;
        cmd_ui_state = {};
        new_ui_cmd = false;
      } else {
        draw_ui_state = pre_disappear_ui_state;
        pre_disappear_ui_state = {};
      }
    } else {
      return;
    }
  } else if (new_ui_cmd) {
    if (cmd_ui_state.disappearing) {
      pre_disappear_ui_state = current_ui_state;
    }
    draw_ui_state = cmd_ui_state;
    cmd_ui_state = {};
    new_ui_cmd = false;
  } else {
    return;
  }

  log_d("[DISP] update: page: %d; disappearing: %d, msg: %s", draw_ui_state.page,
        draw_ui_state.disappearing, draw_ui_state.message.c_str());

  redraw_in_progress = true;
  switch (draw_ui_state.page) {
    case DisplayPage::EMPTY:
      draw_white();
      break;
    case DisplayPage::MAIN:
      draw_main();
      break;
    case DisplayPage::INFO:
      draw_info();
      break;
    case DisplayPage::MESSAGE:
      draw_message(draw_ui_state.message);
      break;
    case DisplayPage::MESSAGE_LARGE:
      draw_message(draw_ui_state.message, false, true);
      break;
    case DisplayPage::ERROR:
      draw_message(draw_ui_state.message, true, false);
      break;
    case DisplayPage::WELCOME:
      draw_welcome();
      break;
    case DisplayPage::AP_CONFIG:
      draw_ap_config();
      break;
    case DisplayPage::WEB_CONFIG:
      draw_web_config();
      break;
    case DisplayPage::TEST:
      draw_test();
      break;
    case DisplayPage::TEST_INV:
      draw_test(true);
      break;
  }
  current_ui_state = draw_ui_state;
  current_ui_state.appear_time = millis();
  draw_ui_state = {};
  redraw_in_progress = false;

  if (state == State::ENDING) {
    disp->hibernate();
    state = State::IDLE;
    log_i("[DISP] ended.");
  }
}

void Display::disp_message(String message, uint32_t duration) {
  UIState new_cmd_state{.page = DisplayPage::MESSAGE, .message = message};
  if (duration > 0) {
    new_cmd_state.disappearing = true;
    new_cmd_state.disappear_timeout = duration;
  }
  set_cmd_state(new_cmd_state);
}

void Display::disp_message_large(String message, uint32_t duration) {
  UIState new_cmd_state{.page = DisplayPage::MESSAGE_LARGE,
                             .message = message};
  if (duration > 0) {
    new_cmd_state.disappearing = true;
    new_cmd_state.disappear_timeout = duration;
  }
  set_cmd_state(new_cmd_state);
}
void Display::disp_error(String message, uint32_t duration) {
  UIState new_cmd_state{.page = DisplayPage::ERROR, .message = message};
  if (duration > 0) {
    new_cmd_state.disappearing = true;
    new_cmd_state.disappear_timeout = duration;
  }
  set_cmd_state(new_cmd_state);
}

void Display::disp_main() {
  UIState new_cmd_state{.page = DisplayPage::MAIN};
  set_cmd_state(new_cmd_state);
}

void Display::disp_info() {
  UIState new_cmd_state{.page = DisplayPage::INFO};
  set_cmd_state(new_cmd_state);
}

void Display::disp_welcome() {
  UIState new_cmd_state{.page = DisplayPage::WELCOME};
  set_cmd_state(new_cmd_state);
}

void Display::disp_ap_config() {
  UIState new_cmd_state{.page = DisplayPage::AP_CONFIG};
  set_cmd_state(new_cmd_state);
}

void Display::disp_web_config() {
  UIState new_cmd_state{.page = DisplayPage::WEB_CONFIG};
  set_cmd_state(new_cmd_state);
}

void Display::disp_test(bool invert) {
UIState new_cmd_state{};
if (!invert) {
  new_cmd_state.page = DisplayPage::TEST;
} else {
  new_cmd_state.page = DisplayPage::TEST_INV;
}
set_cmd_state(new_cmd_state);
}

UIState Display::get_ui_state() { return current_ui_state; }

void Display::init_ui_state(UIState ui_state) {
  current_ui_state = ui_state;
}

Display::State Display::get_state() { return state; }

void Display::set_cmd_state(UIState cmd) {
  cmd_ui_state = cmd;
  new_ui_cmd = true;
}

void Display::draw_message(String message, bool error, bool large) {
  disp->setRotation(0);
  disp->setTextColor(text_color);
  disp->setTextWrap(true);
  disp->setFont(&FreeMono9pt7b);
  disp->setFullWindow();

  disp->fillScreen(bg_color);

  if (!error) {
    if (!large) {
      disp->setFont(&FreeMono9pt7b);
      disp->setCursor(0, 20);
    } else {
      disp->setFont(&FreeMonoBold12pt7b);
      disp->setCursor(0, 30);
    }
    disp->print(message);
  } else {
    disp->setFont(&FreeMonoBold12pt7b);
    disp->setCursor(0, 0);
    int16_t x, y;
    uint16_t w, h;
    String text = "ERROR";
    disp->getTextBounds(text, 0, 0, &x, &y, &w, &h);
    disp->setCursor(WIDTH / 2 - w / 2, 20);
    disp->print(text);
    disp->setFont(&FreeMono9pt7b);
    disp->setCursor(0, 60);
    disp->print(message);
  }
  disp->display();
}

void Display::draw_main() {
  const uint8_t num_buttons = 6;
  const uint16_t min_btn_clearance = 14;
  const uint16_t h_padding = 5;
  const uint16_t W = WIDTH / 2;
  const uint16_t heights[] = {
      static_cast<uint16_t>(round(HEIGHT / 12.)),
      static_cast<uint16_t>(round(HEIGHT / 12. + HEIGHT / 6.)),
      static_cast<uint16_t>(round(HEIGHT / 12. + 2 * HEIGHT / 6.)),
      static_cast<uint16_t>(round(HEIGHT / 12. + 3 * HEIGHT / 6.)),
      static_cast<uint16_t>(round(HEIGHT / 12. + 4 * HEIGHT / 6.)),
      static_cast<uint16_t>(round(HEIGHT / 12. + 5 * HEIGHT / 6.))};

  uint16_t w, h;

  disp->setRotation(0);
  disp->setFullWindow();

  u8g2.setFontMode(1);
  u8g2.setForegroundColor(text_color);
  u8g2.setBackgroundColor(bg_color);

  disp->fillScreen(bg_color);

  // charging line
  if(device_state.charging) {
    disp->fillRect(12, HEIGHT - 3, WIDTH - 24, 3, text_color);
  }

  // Loop through buttons
  for (uint16_t i = 0; i < num_buttons; i++) {
    String t = device_state.get_btn_label(i);

    u8g2.setFont(u8g2_font_helvB24_te);
    w = u8g2.getUTF8Width(t.c_str());
    h = u8g2.getFontAscent();
    if (w >= WIDTH - min_btn_clearance) {
      u8g2.setFont(u8g2_font_helvB18_te);
      w = u8g2.getUTF8Width(t.c_str());
      h = u8g2.getFontAscent();
      if (w >= WIDTH - min_btn_clearance) {
        t = t.substring(0, t.length() - 1) + ".";
        while (1) {
          w = u8g2.getUTF8Width(t.c_str());
          h = u8g2.getFontAscent();
          if (w >= WIDTH - min_btn_clearance) {
            t = t.substring(0, t.length() - 2) + ".";
          } else {
            break;
          }
        }
      }
    }
    int16_t w_pos, h_pos;
    if (i % 2 == 0) {
      w_pos = h_padding;
    } else {
      w_pos = WIDTH - w - h_padding;
    }
    h_pos = heights[i] + h / 2;
    u8g2.setCursor(w_pos, h_pos);
    u8g2.print(t);
  }
  disp->display();
}

void Display::draw_info() {
  disp->setRotation(0);
  disp->setTextColor(text_color);
  disp->setTextWrap(false);
  disp->setFullWindow();

  disp->fillScreen(bg_color);
  disp->setCursor(0, 0);

  int16_t x, y;
  uint16_t w, h;
  String text;

  text = "- Temp -";
  disp->setFont(&FreeMono9pt7b);
  disp->getTextBounds(text, 0, 0, &x, &y, &w, &h);
  disp->setCursor(WIDTH / 2 - w / 2, 30);
  disp->print(text);

  text = String(device_state.temperature, 1) + String(" C");
  disp->setFont(&FreeSansBold18pt7b);
  disp->getTextBounds(text, 0, 0, &x, &y, &w, &h);
  disp->setCursor(WIDTH / 2 - w / 2 - 2, 70);
  disp->print(text);

  text = "- Humd -";
  disp->setFont(&FreeMono9pt7b);
  disp->getTextBounds(text, 0, 0, &x, &y, &w, &h);
  disp->setCursor(WIDTH / 2 - w / 2, 129);
  disp->print(text);

  text = String(device_state.humidity, 0) + String(" %");
  disp->setFont(&FreeSansBold18pt7b);
  disp->getTextBounds(text, 0, 0, &x, &y, &w, &h);
  disp->setCursor(WIDTH / 2 - w / 2 - 2, 169);
  disp->print(text);

  text = "- Batt -";
  disp->setFont(&FreeMono9pt7b);
  disp->getTextBounds(text, 0, 0, &x, &y, &w, &h);
  disp->setCursor(WIDTH / 2 - w / 2, 228);
  disp->print(text);

  if (device_state.battery_present) {
    text = String(device_state.battery_pct) + String(" %");
  } else {
    text = "-";
  }
  disp->setFont(&FreeSansBold18pt7b);
  disp->getTextBounds(text, 0, 0, &x, &y, &w, &h);
  disp->setCursor(WIDTH / 2 - w / 2 - 2, 268);
  disp->print(text);

  // device name
  disp->setFont();
  disp->setTextSize(1);
  disp->setCursor(0, 288);
  disp->print(device_state.device_name);

  disp->display();
}

void Display::draw_welcome() {
  disp->setRotation(0);
  disp->setTextColor(text_color);
  disp->setTextWrap(false);
  disp->setFullWindow();

  disp->fillScreen(bg_color);
  disp->setCursor(0, 0);

  int16_t x, y;
  uint16_t w, h;
  String text;

  text = "Home Buttons";
  disp->setFont(&FreeSansBold9pt7b);
  disp->getTextBounds(text, 0, 0, &x, &y, &w, &h);
  disp->setCursor(WIDTH / 2 - w / 2, 40);
  disp->print(text);

  disp->drawBitmap(54, 52, hb_logo_20x21px, 20, 21, GxEPD_BLACK);

  disp->setFont(&FreeSansBold9pt7b);
  disp->getTextBounds("--------------------", 0, 0, &x, &y, &w, &h);
  disp->setCursor(WIDTH / 2 - w / 2, 102);
  disp->print("--------------------");

  uint8_t version = 6;  // 41x41px
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(version)];
  qrcode_initText(&qrcode, qrcodeData, version, ECC_HIGH, DOCS_LINK);

  text = "Setup guide:";
  disp->setFont(&FreeSans9pt7b);
  disp->getTextBounds(text, 0, 0, &x, &y, &w, &h);
  disp->setCursor(WIDTH / 2 - w / 2, 145);
  disp->print(text);

  uint16_t qr_x = 23;
  uint16_t qr_y = 165;
  for (uint8_t y = 0; y < qrcode.size; y++) {
    // Each horizontal module
    for (uint8_t x = 0; x < qrcode.size; x++) {
      // Display each module
      if (qrcode_getModule(&qrcode, x, y)) {
        disp->drawRect(qr_x + x * 2, qr_y + y * 2, 2, 2, GxEPD_BLACK);
      }
    }
  }

  disp->setFont();
  disp->setTextSize(1);

  disp->setCursor(0, 265);
  String sw_ver = String("Software: ") + SW_VERSION;
  disp->print(sw_ver);

  disp->setCursor(0, 275);
  String model_info =
      String("Model: ") + device_state.model_id + " rev " + device_state.hw_version;
  disp->print(model_info);

  disp->setCursor(0, 285);
  disp->print(device_state.unique_id);

  disp->display();
}

void Display::draw_ap_config() {
  String contents = String("WIFI:T:WPA;S:") + device_state.ap_ssid +
                    ";P:" + device_state.ap_password + ";;";

  uint8_t version = 6;  // 41x41px
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(version)];
  qrcode_initText(&qrcode, qrcodeData, version, ECC_HIGH, contents.c_str());

  disp->setRotation(0);
  disp->setTextColor(text_color);
  disp->setTextWrap(false);
  disp->setFont(&FreeSans9pt7b);
  disp->setFullWindow();

  disp->fillScreen(bg_color);

  int16_t x, y;
  uint16_t w, h;
  String text;

  text = "Scan:";
  disp->getTextBounds(text, 0, 0, &x, &y, &w, &h);
  disp->setCursor(WIDTH / 2 - w / 2, 20);
  disp->print(text);

  uint16_t qr_x = 23;
  uint16_t qr_y = 35;
  for (uint8_t y = 0; y < qrcode.size; y++) {
    // Each horizontal module
    for (uint8_t x = 0; x < qrcode.size; x++) {
      // Display each module
      if (qrcode_getModule(&qrcode, x, y)) {
        disp->drawRect(qr_x + x * 2, qr_y + y * 2, 2, 2, GxEPD_BLACK);
      }
    }
  }
  text = "------ or ------";
  disp->setFont(&FreeSansBold9pt7b);
  disp->getTextBounds(text, 0, 0, &x, &y, &w, &h);
  disp->setCursor(WIDTH / 2 - w / 2, 153);
  disp->print(text);

  text = "Connect to:";
  disp->setFont(&FreeSans9pt7b);
  disp->getTextBounds(text, 0, 0, &x, &y, &w, &h);
  disp->setCursor(WIDTH / 2 - w / 2, 190);
  disp->print(text);

  disp->setCursor(0, 220);
  disp->print("Wi-Fi:");
  disp->setFont(&FreeSansBold9pt7b);
  disp->setCursor(0, 235);
  disp->print(device_state.ap_ssid.c_str());

  disp->setFont(&FreeSans9pt7b);
  disp->setCursor(0, 260);
  disp->print("Password:");
  disp->setFont(&FreeSansBold9pt7b);
  disp->setCursor(0, 275);
  disp->print(device_state.ap_password.c_str());

  disp->display();
}

void Display::draw_web_config() {
  String contents = String("http://") + device_state.ip;

  uint8_t version = 6;  // 41x41px
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(version)];
  qrcode_initText(&qrcode, qrcodeData, version, ECC_HIGH, contents.c_str());

  disp->setRotation(0);
  disp->setTextColor(text_color);
  disp->setTextWrap(false);
  disp->setFont(&FreeSans9pt7b);
  disp->setFullWindow();

  disp->fillScreen(bg_color);

  int16_t x, y;
  uint16_t w, h;
  String text;

  text = "Scan:";
  disp->getTextBounds(text, 0, 0, &x, &y, &w, &h);
  disp->setCursor(WIDTH / 2 - w / 2, 20);
  disp->print(text);

  uint16_t qr_x = 23;
  uint16_t qr_y = 35;

  for (uint8_t y = 0; y < qrcode.size; y++) {
    // Each horizontal module
    for (uint8_t x = 0; x < qrcode.size; x++) {
      // Display each module
      if (qrcode_getModule(&qrcode, x, y)) {
        disp->drawRect(qr_x + x * 2, qr_y + y * 2, 2, 2, GxEPD_BLACK);
      }
    }
  }
  text = "------ or ------";
  disp->setFont(&FreeSansBold9pt7b);
  disp->getTextBounds(text, 0, 0, &x, &y, &w, &h);
  disp->setCursor(WIDTH / 2 - w / 2, 153);
  disp->print(text);

  text = "Go to:";
  disp->setFont(&FreeSans9pt7b);
  disp->getTextBounds(text, 0, 0, &x, &y, &w, &h);
  disp->setCursor(WIDTH / 2 - w / 2, 200);
  disp->print(text);

  disp->setFont(&FreeSansBold9pt7b);
  disp->setCursor(0, 240);
  disp->print("http://");
  disp->setCursor(0, 260);
  disp->print(device_state.ip.c_str());

  disp->display();
}

void Display::draw_test(bool invert) {
  uint16_t fg, bg;
  if (!invert) {
    fg = GxEPD_BLACK;
    bg = GxEPD_WHITE;
  } else {
    fg = GxEPD_WHITE;
    bg = GxEPD_BLACK;
  }

  disp->setRotation(0);
  disp->setTextColor(fg);
  disp->setTextWrap(true);
  disp->setFont(&FreeSansBold24pt7b);
  disp->setFullWindow();

  disp->fillScreen(bg);

  int16_t x, y;
  uint16_t w, h;
  String text;

  text = "TEST";
  disp->getTextBounds(text, 0, 0, &x, &y, &w, &h);

  disp->setCursor(WIDTH / 2 - w / 2, 90);
  disp->print(text);

  disp->setCursor(WIDTH / 2 - w / 2, 170);
  disp->print(text);

  disp->setCursor(WIDTH / 2 - w / 2, 250);
  disp->print(text);

  disp->display();
}

void Display::draw_white() {
  disp->setFullWindow();
  disp->fillScreen(GxEPD_WHITE);
  disp->display();
}

void Display::draw_black() {
  disp->setFullWindow();
  disp->fillScreen(GxEPD_BLACK);
  disp->display();
}
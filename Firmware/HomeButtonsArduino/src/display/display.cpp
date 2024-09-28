#include "display.h"

#include <GxEPD2_BW.h>
#include <SPIFFS.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <qrcode.h>

#include "bitmaps.h"
#include "config.h"
#include "hardware.h"

#if defined(HOME_BUTTONS_ORIGINAL)
static constexpr uint16_t ROTATION = 0;
static constexpr uint16_t WIDTH = 128;
static constexpr uint16_t HEIGHT = 296;
#elif defined(HOME_BUTTONS_MINI)
static constexpr uint16_t ROTATION = 0;
static constexpr int WIDTH = 200;
constexpr int HEIGHT = 200;
#elif defined(HOME_BUTTONS_PRO)
static constexpr uint16_t ROTATION = 0;
static constexpr uint16_t WIDTH = 400;
static constexpr uint16_t HEIGHT = 300;
#endif

uint16_t read16(File &f) {
  // BMP data is stored little-endian, same as Arduino.
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read();  // LSB
  ((uint8_t *)&result)[1] = f.read();  // MSB

  return result;
}

uint32_t read32(File &f) {
  // BMP data is stored little-endian, same as Arduino.
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read();  // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read();  // MSB
  return result;
}

LabelType Display::get_label_type(ButtonLabel label) {
  if (label.substring(0, 4) == "mdi:") {
    if (label.index_of(' ') > 0) {
      return LabelType::Mixed;
    } else {
      return LabelType::Icon;
    }
  } else {
    return LabelType::Text;
  }
}

MDIName Display::get_mdi_name(ButtonLabel label) {
  if (get_label_type(label) == LabelType::Icon) {
    return MDIName{label.substring(4)};
  } else if (get_label_type(label) == LabelType::Mixed) {
    return MDIName{label.substring(4, label.index_of(' '))};
  } else {
    return MDIName{};
  }
}

ButtonLabel Display::get_text(ButtonLabel label) {
  if (get_label_type(label) == LabelType::Text) {
    return label;
  } else if (get_label_type(label) == LabelType::Mixed) {
    return label.substring(label.index_of(' ') + 1);
  } else {
    return ButtonLabel{};
  }
}

ButtonLabel Display::trim_text(ButtonLabel label, uint16_t max_width) {
  uint16_t w = u8g2.getUTF8Width(label.c_str());
  while (w > max_width) {
    label = label.substring(0, label.length() - 2) + ".";
    w = u8g2.getUTF8Width(label.c_str());
  }
  return label;
}

void Display::begin(HardwareDefinition &HW) {
  if (state != State::IDLE) return;
  disp = new GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS,
                                  MAX_HEIGHT(GxEPD2_DRIVER_CLASS)>(
      GxEPD2_DRIVER_CLASS(/*CS=*/HW.EINK_CS, /*DC=*/HW.EINK_DC,
                          /*RST=*/HW.EINK_RST, /*BUSY=*/HW.EINK_BUSY));
  disp->init(0, false);
  u8g2.begin(*disp);
  current_ui_state = {};
  cmd_ui_state = {};
  draw_ui_state = {};
  pre_disappear_ui_state = {};
  state = State::ACTIVE;
  info("begin");
}

void Display::end() {
  if (state != State::ACTIVE) return;
  state = State::CMD_END;
  debug("cmd end");
}

void Display::update() {
  if (state == State::IDLE) return;

  if (state == State::CMD_END && !new_ui_cmd) {
    state = State::ENDING;
    if (current_ui_state.disappearing) {
      draw_ui_state = pre_disappear_ui_state;
    } else {
      disp->hibernate();
      state = State::IDLE;
      info("ended.");
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

  debug("update: page: %d; disappearing: %d, msg: %s",
        static_cast<int>(draw_ui_state.page), draw_ui_state.disappearing,
        draw_ui_state.message.c_str());

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
    case DisplayPage::DEVICE_INFO:
      draw_device_info();
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
    case DisplayPage::SETTINGS:
      draw_settings();
      break;
    case DisplayPage::AP_CONFIG:
      draw_ap_config();
      break;
    case DisplayPage::WEB_CONFIG:
      draw_web_config();
      break;
    case DisplayPage::TEST:
      draw_test(draw_ui_state.message.c_str(), draw_ui_state.mdi_name.c_str(),
                draw_ui_state.mdi_size);
      break;
  }
  current_ui_state = draw_ui_state;
  current_ui_state.appear_time = millis();
  draw_ui_state = {};
  redraw_in_progress = false;

  if (state == State::ENDING) {
    disp->hibernate();
    state = State::IDLE;
    info("ended.");
  }
}

void Display::disp_message(const char *message, uint32_t duration) {
  UIState new_cmd_state{DisplayPage::MESSAGE, UIState::MessageType{message}};
  if (duration > 0) {
    new_cmd_state.disappearing = true;
    new_cmd_state.disappear_timeout = duration;
  }
  set_cmd_state(new_cmd_state);
}

void Display::disp_message_large(const char *message, uint32_t duration) {
  UIState new_cmd_state{DisplayPage::MESSAGE_LARGE,
                        UIState::MessageType{message}};
  if (duration > 0) {
    new_cmd_state.disappearing = true;
    new_cmd_state.disappear_timeout = duration;
  }
  set_cmd_state(new_cmd_state);
}
void Display::disp_error(const char *message, uint32_t duration) {
  UIState new_cmd_state{DisplayPage::ERROR, UIState::MessageType{message}};
  if (duration > 0) {
    new_cmd_state.disappearing = true;
    new_cmd_state.disappear_timeout = duration;
  }
  set_cmd_state(new_cmd_state);
}

void Display::disp_main() {
  UIState new_cmd_state;
  if (device_state_.persisted().setup_done &&
      device_state_.persisted().wifi_done) {
    new_cmd_state = {DisplayPage::MAIN};
  } else {
    new_cmd_state = {DisplayPage::WELCOME};
  }
  set_cmd_state(new_cmd_state);
}

void Display::disp_info() {
  UIState new_cmd_state{DisplayPage::INFO};
  set_cmd_state(new_cmd_state);
}

void Display::disp_device_info() {
  UIState new_cmd_state{DisplayPage::DEVICE_INFO};
  set_cmd_state(new_cmd_state);
}

void Display::disp_welcome() {
  UIState new_cmd_state{DisplayPage::WELCOME};
  set_cmd_state(new_cmd_state);
}

void Display::disp_settings() {
  UIState new_cmd_state{DisplayPage::SETTINGS};
  set_cmd_state(new_cmd_state);
}
void Display::disp_ap_config() {
  UIState new_cmd_state{DisplayPage::AP_CONFIG};
  set_cmd_state(new_cmd_state);
}

void Display::disp_web_config() {
  UIState new_cmd_state{DisplayPage::WEB_CONFIG};
  set_cmd_state(new_cmd_state);
}

void Display::disp_test(const char *text, const char *mdi_name,
                        uint16_t mdi_size) {
  UIState new_cmd_state{};
  new_cmd_state.page = DisplayPage::TEST;
  new_cmd_state.message = UIState::MessageType{text};
  new_cmd_state.mdi_name = MDIName{mdi_name};
  new_cmd_state.mdi_size = mdi_size;
  set_cmd_state(new_cmd_state);
}

UIState Display::get_ui_state() { return current_ui_state; }

void Display::init_ui_state(UIState ui_state) { current_ui_state = ui_state; }

Display::State Display::get_state() { return state; }

void Display::set_cmd_state(UIState cmd) {
  cmd_ui_state = cmd;
  new_ui_cmd = true;
}

void Display::draw_message(const UIState::MessageType &message, bool error,
                           bool large) {
  disp->setRotation(ROTATION);
  disp->setFullWindow();

  u8g2.setFontMode(1);
  u8g2.setForegroundColor(text_color);
  u8g2.setBackgroundColor(bg_color);

  disp->fillScreen(bg_color);

#if defined(HOME_BUTTONS_ORIGINAL)
  if (!error) {
    if (!large) {
      u8g2.setFont(u8g2_font_courR12_tr);
      u8g2.setCursor(0, 20);
    } else {
      u8g2.setFont(u8g2_font_helvB18_te);
      u8g2.setCursor(0, 30);
    }
    u8g2.print(message.c_str());
  } else {
    u8g2.setFont(u8g2_font_helvB12_tr);
    const char *text = "ERROR";
    uint16_t w = u8g2.getUTF8Width(text);
    u8g2.setCursor(WIDTH / 2 - w / 2, 20);
    u8g2.print(text);
    u8g2.setFont(u8g2_font_courR12_tr);
    u8g2.setCursor(0, 60);
    u8g2.print(message.c_str());
  }

#elif defined(HOME_BUTTONS_MINI)
  if (!error) {
    if (!large) {
      u8g2.setFont(u8g2_font_courR18_tf);
      u8g2.setCursor(0, 20);
    } else {
      u8g2.setFont(u8g2_font_helvB24_tr);
      u8g2.setCursor(0, 30);
    }
    u8g2.print(message.c_str());
  } else {
    u8g2.setFont(u8g2_font_helvB18_tr);
    const char *text = "ERROR";
    uint16_t w = u8g2.getUTF8Width(text);
    u8g2.setCursor(WIDTH / 2 - w / 2, 30);
    u8g2.print(text);
    u8g2.setFont(u8g2_font_courR18_tf);
    u8g2.setCursor(0, 70);
    u8g2.print(message.c_str());
  }

#elif defined(HOME_BUTTONS_PRO)
  if (!error) {
    if (!large) {
      u8g2.setFont(u8g2_font_courR12_tr);
      u8g2.setCursor(0, 20);
    } else {
      u8g2.setFont(u8g2_font_helvB18_te);
      u8g2.setCursor(0, 30);
    }
    u8g2.print(message.c_str());
  } else {
    u8g2.setFont(u8g2_font_helvB12_tr);
    const char *text = "ERROR";
    uint16_t w = u8g2.getUTF8Width(text);
    u8g2.setCursor(WIDTH / 2 - w / 2, 20);
    u8g2.print(text);
    u8g2.setFont(u8g2_font_courR12_tr);
    u8g2.setCursor(0, 60);
    u8g2.print(message.c_str());
  }
#endif

  disp->display();
}

void Display::draw_main() {
  disp->setRotation(ROTATION);
  disp->setFullWindow();

  u8g2.setFontMode(1);
  u8g2.setForegroundColor(text_color);
  u8g2.setBackgroundColor(bg_color);

  disp->fillScreen(bg_color);

#if defined(HOME_BUTTONS_ORIGINAL)
  const uint16_t min_btn_clearance = 14;
  const uint16_t h_padding = 5;

  // charging line
  if (device_state_.sensors().charging) {
    disp->fillRect(12, HEIGHT - 3, WIDTH - 24, 3, text_color);
  }

  mdi_.begin();
  LabelType label_type[NUM_BUTTONS] = {};
  for (uint16_t i = 0; i < NUM_BUTTONS; i++) {
    ButtonLabel label = device_state_.get_btn_label(i + 1);
    label_type[i] = get_label_type(label);
  }

  // Loop through buttons
  for (uint16_t i = 0; i < NUM_BUTTONS; i++) {
    ButtonLabel label = device_state_.get_btn_label(i + 1);

    if (label_type[i] == LabelType::Icon) {
      MDIName icon = get_mdi_name(label);

      // make smaller if opposite is text or mixed
      uint16_t size;
      bool small;
      if (i % 2 == 0) {
        if (label_type[i + 1] == LabelType::Text ||
            label_type[i + 1] == LabelType::Mixed) {
          size = 48;
          small = true;
        } else {
          size = 64;
          small = false;
        }
      } else {
        if (label_type[i - 1] == LabelType::Text ||
            label_type[i - 1] == LabelType::Mixed) {
          size = 48;
          small = true;
        } else {
          size = 64;
          small = false;
        }
      }

      // calculate icon position on display
      uint16_t x = i % 2 == 0 ? 0 : WIDTH - size;
      uint16_t y;
      if (i < 2) {
        if (small) {
          y = static_cast<uint16_t>(round(HEIGHT / 12. + i * HEIGHT / 6.)) -
              size / 2;
        } else {
          y = 17;
        }
      } else if (i < 4) {
        if (small) {
          y = static_cast<uint16_t>(round(HEIGHT / 12. + i * HEIGHT / 6.)) -
              size / 2;
        } else {
          y = 116;
        }
      } else {
        if (small) {
          y = static_cast<uint16_t>(round(HEIGHT / 12. + i * HEIGHT / 6.)) -
              size / 2;
        } else {
          y = 215;
        }
      }
      draw_mdi(icon.c_str(), size, x, y);
    } else if (label_type[i] == LabelType::Mixed) {
      MDIName icon = get_mdi_name(label);
      ButtonLabel text = get_text(label);
      uint16_t icon_size = 48;
      uint16_t x = i % 2 == 0 ? 0 : WIDTH - icon_size;
      uint16_t y =
          static_cast<uint16_t>(round(HEIGHT / 12. + i * HEIGHT / 6.)) -
          icon_size / 2;

      draw_mdi(icon.c_str(), icon_size, x, y);
      // draw text
      uint16_t max_text_width = WIDTH - icon_size - h_padding;
      if (text.index_of('_') == 0) {
        // force small font
        text = text.substring(1);
        u8g2.setFont(u8g2_font_helvB18_te);
      } else {
        u8g2.setFont(u8g2_font_helvB24_te);
      }
      uint16_t w, h;
      w = u8g2.getUTF8Width(text.c_str());
      h = u8g2.getFontAscent();
      if (w >= max_text_width) {
        u8g2.setFont(u8g2_font_helvB18_te);
        w = u8g2.getUTF8Width(text.c_str());
        h = u8g2.getFontAscent();
        if (w >= max_text_width) {
          text = text.substring(0, text.length() - 1) + ".";
          while (1) {
            w = u8g2.getUTF8Width(text.c_str());
            h = u8g2.getFontAscent();
            if (w >= max_text_width) {
              text = text.substring(0, text.length() - 2) + ".";
            } else {
              break;
            }
          }
        }
      }
      x = i % 2 == 0 ? icon_size + h_padding
                     : WIDTH - icon_size - w - h_padding;
      y = static_cast<uint16_t>(round(HEIGHT / 12. + i * HEIGHT / 6.)) + h / 2;
      u8g2.setCursor(x, y);
      u8g2.print(text.c_str());
    } else {
      uint16_t max_label_width = WIDTH - min_btn_clearance;
      if (label.index_of('_') == 0) {
        // force small font
        label = label.substring(1);
        u8g2.setFont(u8g2_font_helvB18_te);
      } else {
        u8g2.setFont(u8g2_font_helvB24_te);
      }
      uint16_t w, h;
      w = u8g2.getUTF8Width(label.c_str());
      h = u8g2.getFontAscent();
      if (w >= max_label_width) {
        u8g2.setFont(u8g2_font_helvB18_te);
        w = u8g2.getUTF8Width(label.c_str());
        h = u8g2.getFontAscent();
        if (w >= max_label_width) {
          label = label.substring(0, label.length() - 1) + ".";
          while (1) {
            w = u8g2.getUTF8Width(label.c_str());
            h = u8g2.getFontAscent();
            if (w >= max_label_width) {
              label = label.substring(0, label.length() - 2) + ".";
            } else {
              break;
            }
          }
        }
      }
      int16_t x, y;
      if (i % 2 == 0) {
        x = h_padding;
      } else {
        x = WIDTH - w - h_padding;
      }
      y = static_cast<uint16_t>(round(HEIGHT / 12. + i * HEIGHT / 6.)) + h / 2;
      u8g2.setCursor(x, y);
      u8g2.print(label.c_str());
    }
  }
  mdi_.end();

#elif defined(HOME_BUTTONS_MINI)
  mdi_.begin();
  // Loop through buttons
  for (uint16_t i = 0; i < NUM_BUTTONS; i++) {
    ButtonLabel label = device_state_.get_btn_label(i + 1);
    uint16_t size = 100;
    uint16_t x = i % 2 == 0 ? 0 : WIDTH - size;
    uint16_t y = i < 2 ? 0 : HEIGHT - size;
    MDIName icon = get_mdi_name(label);
    draw_mdi(icon.c_str(), size, x, y);
  }
  mdi_.end();

#elif defined(HOME_BUTTONS_PRO)
  uint16_t tile_width = 132;
  uint16_t tile_height = 100;
  for (uint16_t i = 0; i < NUM_BUTTONS; i++) {
    ButtonLabel label = device_state_.get_btn_label(i + 1);

    ButtonTile tile = {};
    tile.label_type = get_label_type(label);
    tile.text = get_text(label);
    tile.mdi_name = get_mdi_name(label);
    tile.width = tile_width;
    tile.height = tile_height;

    int16_t x = 2 + i % 3 * tile.width;
    int16_t y = i / 3 * tile.height;
    tile.draw(*this, x, y, text_color);
  }
  // grid
  disp->drawFastHLine(0, tile_height, WIDTH, text_color);
  disp->drawFastHLine(0, 2 * tile_height, WIDTH, text_color);
  disp->drawFastVLine(133, 0, HEIGHT, text_color);
  disp->drawFastVLine(266, 0, HEIGHT, text_color);

#endif

  disp->display();
}

void Display::draw_info() {
  disp->setRotation(ROTATION);
  disp->setFullWindow();

  u8g2.setFontMode(1);
  u8g2.setForegroundColor(text_color);
  u8g2.setBackgroundColor(bg_color);

  disp->fillScreen(bg_color);

  UIState::MessageType text;

#if defined(HOME_BUTTONS_ORIGINAL)
  uint16_t w;

  text = "- Temp -";
  u8g2.setFont(u8g2_font_courR12_tr);
  w = u8g2.getUTF8Width(text.c_str());
  u8g2.setCursor(WIDTH / 2 - w / 2, 30);
  u8g2.print(text.c_str());

  text = UIState::MessageType("%.1f %s", device_state_.sensors().temperature,
                              device_state_.get_temp_unit().c_str());
  u8g2.setFont(u8g2_font_helvB24_te);
  w = u8g2.getUTF8Width(text.c_str());
  u8g2.setCursor(WIDTH / 2 - w / 2 - 2, 70);
  u8g2.print(text.c_str());

  text = "- Humd -";
  u8g2.setFont(u8g2_font_courR12_tr);
  w = u8g2.getUTF8Width(text.c_str());
  u8g2.setCursor(WIDTH / 2 - w / 2, 129);
  u8g2.print(text.c_str());

  text = UIState::MessageType("%.0f %%", device_state_.sensors().humidity);
  u8g2.setFont(u8g2_font_helvB24_te);
  w = u8g2.getUTF8Width(text.c_str());
  u8g2.setCursor(WIDTH / 2 - w / 2 - 2, 169);
  u8g2.print(text.c_str());

  text = "- Batt -";
  u8g2.setFont(u8g2_font_courR12_tr);
  w = u8g2.getUTF8Width(text.c_str());
  u8g2.setCursor(WIDTH / 2 - w / 2, 228);
  u8g2.print(text.c_str());

  if (device_state_.sensors().battery_present) {
    text = UIState::MessageType("%d %%", device_state_.sensors().battery_pct);
  } else {
    text = "-";
  }
  u8g2.setFont(u8g2_font_helvB24_te);
  w = u8g2.getUTF8Width(text.c_str());
  u8g2.setCursor(WIDTH / 2 - w / 2 - 2, 268);
  u8g2.print(text.c_str());

#elif defined(HOME_BUTTONS_MINI)
  u8g2.setFont(u8g2_font_helvB24_tr);

  disp->drawXBitmap(5, 4, thermometer_64x64, 64, 64, text_color);
  text = UIState::MessageType("%.1f %s", device_state_.sensors().temperature,
                              device_state_.get_temp_unit().c_str());
  u8g2.setCursor(85, 50);
  u8g2.print(text.c_str());

  disp->drawXBitmap(5, 68, water_percent_64x64, 64, 64, text_color);
  text = UIState::MessageType("%.0f %%", device_state_.sensors().humidity);
  u8g2.setCursor(85, 116);
  u8g2.print(text.c_str());

  disp->drawXBitmap(5, 132, battery_64x64, 64, 64, text_color);
  text = UIState::MessageType("%d %%", device_state_.sensors().battery_pct);
  u8g2.setCursor(85, 180);
  u8g2.print(text.c_str());

#elif defined(HOME_BUTTONS_PRO)
  u8g2.setFont(u8g2_font_helvB24_tr);

  disp->drawXBitmap(100, 30, thermometer_64x64, 64, 64, text_color);
  text = UIState::MessageType("%.1f %s", device_state_.sensors().temperature,
                              device_state_.get_temp_unit().c_str());
  int8_t ascent = u8g2.getFontAscent();
  u8g2.setCursor(180, 30 + 64 / 2 + ascent / 2);
  u8g2.print(text.c_str());

  disp->drawXBitmap(100, 110, water_percent_64x64, 64, 64, text_color);
  text = UIState::MessageType("%.0f %%", device_state_.sensors().humidity);
  u8g2.setCursor(180, 110 + 64 / 2 + ascent / 2);
  u8g2.print(text.c_str());

  disp->drawLine(20, 200, 380, 200, text_color);

  // device info
  disp->drawXBitmap(6, 210, hb_logo_64x64, 64, 64, text_color);

  u8g2.setFont(u8g2_font_profont12_tr);

  u8g2.setCursor(75, 220);
  u8g2.print(device_state_.device_name().c_str());

  UIState::MessageType sw_ver = UIState::MessageType("SW: ") + SW_VERSION;
  u8g2.setCursor(75, 232);
  u8g2.print(sw_ver.c_str());

  UIState::MessageType model_info = UIState::MessageType("Model: ") +
                                    device_state_.factory().model_id.c_str() +
                                    " rev " +
                                    device_state_.factory().hw_version.c_str();
  u8g2.setCursor(75, 244);
  u8g2.print(model_info.c_str());

  u8g2.setCursor(75, 256);
  u8g2.print(device_state_.factory().unique_id.c_str());

  UIState::MessageType ip_info =
      UIState::MessageType("IP: %s", device_state_.ip());
  u8g2.setCursor(75, 268);
  u8g2.print(ip_info.c_str());

  // settings icon
  disp->drawXBitmap(WIDTH - 70, HEIGHT - 90, cog_64x64, 64, 64, text_color);
  u8g2.setCursor(357, 285);
  u8g2.print("5s");

  // up chevron
  disp->drawXBitmap(WIDTH / 2 - 32 / 2, HEIGHT - 28, chevron_up_32x32, 32, 32,
                    text_color);
#endif

  disp->display();
}

void Display::draw_device_info() {
  disp->setRotation(ROTATION);
  disp->setFullWindow();

  u8g2.setFontMode(1);
  u8g2.setForegroundColor(text_color);
  u8g2.setBackgroundColor(bg_color);

  disp->fillScreen(bg_color);

#if defined(HOME_BUTTONS_ORIGINAL)
  disp->drawXBitmap(40, 0, hb_logo_48x48, 48, 48, text_color);

  u8g2.setFont(u8g2_font_profont12_tr);

  u8g2.setCursor(0, 70);
  u8g2.print(device_state_.device_name().c_str());

  UIState::MessageType sw_ver = UIState::MessageType("SW: ") + SW_VERSION;
  u8g2.setCursor(0, 82);
  u8g2.print(sw_ver.c_str());

  UIState::MessageType model_info = UIState::MessageType("Model: ") +
                                    device_state_.factory().model_id.c_str() +
                                    " rev " +
                                    device_state_.factory().hw_version.c_str();
  u8g2.setCursor(0, 94);
  u8g2.print(model_info.c_str());

  u8g2.setCursor(0, 106);
  u8g2.print(device_state_.factory().unique_id.c_str());

  UIState::MessageType ip_info =
      UIState::MessageType("IP: %s", device_state_.ip());
  u8g2.setCursor(0, 140);
  u8g2.print(ip_info.c_str());

  UIState::MessageType batt_volt;
  if (device_state_.sensors().battery_present) {
    batt_volt = UIState::MessageType("Battery: %.2f V",
                                     device_state_.sensors().battery_voltage);
  } else {
    batt_volt = UIState::MessageType("Battery: -");
  }
  u8g2.setCursor(0, 152);
  u8g2.print(batt_volt.c_str());

#elif defined(HOME_BUTTONS_MINI)
  disp->drawXBitmap(76, 0, hb_logo_48x48, 48, 48, text_color);

  u8g2.setFont(u8g2_font_profont17_tr);

  u8g2.setCursor(0, 70);
  u8g2.print(device_state_.device_name().c_str());

  UIState::MessageType sw_ver = UIState::MessageType("SW: ") + SW_VERSION;
  u8g2.setCursor(0, 90);
  u8g2.print(sw_ver.c_str());

  UIState::MessageType model_info = UIState::MessageType("Model: ") +
                                    device_state_.factory().model_id.c_str() +
                                    " rev " +
                                    device_state_.factory().hw_version.c_str();
  u8g2.setCursor(0, 110);
  u8g2.print(model_info.c_str());

  u8g2.setCursor(0, 130);
  u8g2.print(device_state_.factory().unique_id.c_str());

  UIState::MessageType ip_info =
      UIState::MessageType("IP: %s", device_state_.ip());
  u8g2.setCursor(0, 160);
  u8g2.print(ip_info.c_str());

  UIState::MessageType batt_volt = UIState::MessageType(
      "Battery: %.2f V", device_state_.sensors().battery_voltage);
  u8g2.setCursor(0, 180);
  u8g2.print(batt_volt.c_str());

#elif defined(HOME_BUTTONS_PRO)
  disp->drawXBitmap(76, 0, hb_logo_48x48, 48, 48, text_color);

  u8g2.setFont(u8g2_font_profont17_tr);

  u8g2.setCursor(0, 70);
  u8g2.print(device_state_.device_name().c_str());

  UIState::MessageType sw_ver = UIState::MessageType("SW: ") + SW_VERSION;
  u8g2.setCursor(0, 90);
  u8g2.print(sw_ver.c_str());

  UIState::MessageType model_info = UIState::MessageType("Model: ") +
                                    device_state_.factory().model_id.c_str() +
                                    " rev " +
                                    device_state_.factory().hw_version.c_str();
  u8g2.setCursor(0, 110);
  u8g2.print(model_info.c_str());

  u8g2.setCursor(0, 130);
  u8g2.print(device_state_.factory().unique_id.c_str());

  UIState::MessageType ip_info =
      UIState::MessageType("IP: %s", device_state_.ip());
  u8g2.setCursor(0, 160);
  u8g2.print(ip_info.c_str());

  UIState::MessageType batt_volt = UIState::MessageType(
      "Battery: %.2f V", device_state_.sensors().battery_voltage);
  u8g2.setCursor(0, 180);
  u8g2.print(batt_volt.c_str());
#endif

  disp->display();
}

void Display::draw_welcome() {
  disp->setRotation(ROTATION);
  disp->setFullWindow();

  u8g2.setFontMode(1);
  u8g2.setForegroundColor(text_color);
  u8g2.setBackgroundColor(bg_color);

  disp->fillScreen(bg_color);

#if defined(HOME_BUTTONS_ORIGINAL)
  uint16_t w;
  const char *text = "Home Buttons";
  u8g2.setFont(u8g2_font_helvB12_tr);
  w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 40);
  u8g2.print(text);

  disp->drawXBitmap(52, 52, hb_logo_24x24, 24, 24, GxEPD_BLACK);

  text = "------------------------";
  u8g2.setFont(u8g2_font_helvB12_tr);
  w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 102);
  u8g2.print(text);

  uint8_t version = 6;  // 41x41px
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(version)];
  qrcode_initText(&qrcode, qrcodeData, version, ECC_HIGH, DOCS_LINK);

  text = "Setup guide:";
  u8g2.setFont(u8g2_font_courR12_tr);
  w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 145);
  u8g2.print(text);

  uint16_t qr_x = 23;
  uint16_t qr_y = 165;
  for (uint8_t y2 = 0; y2 < qrcode.size; y2++) {
    // Each horizontal module
    for (uint8_t x2 = 0; x2 < qrcode.size; x2++) {
      // Display each module
      if (qrcode_getModule(&qrcode, x2, y2)) {
        disp->drawRect(qr_x + x2 * 2, qr_y + y2 * 2, 2, 2, GxEPD_BLACK);
      }
    }
  }

  u8g2.setFont(u8g2_font_profont12_tr);
  UIState::MessageType sw_ver = UIState::MessageType("SW: ") + SW_VERSION;
  u8g2.setCursor(0, 272);
  u8g2.print(sw_ver.c_str());

  UIState::MessageType model_info = UIState::MessageType("Model: ") +
                                    device_state_.factory().model_id.c_str() +
                                    " rev " +
                                    device_state_.factory().hw_version.c_str();
  u8g2.setCursor(0, 283);
  u8g2.print(model_info.c_str());

  u8g2.setCursor(0, 294);
  u8g2.print(device_state_.factory().unique_id.c_str());

#elif defined(HOME_BUTTONS_MINI)
  uint8_t version = 8;  // 49x49px
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(version)];
  qrcode_initText(&qrcode, qrcodeData, version, ECC_HIGH, DOCS_LINK);
  uint16_t qr_x = 2;
  uint16_t qr_y = 2;
  for (uint8_t y2 = 0; y2 < qrcode.size; y2++) {
    // Each horizontal module
    for (uint8_t x2 = 0; x2 < qrcode.size; x2++) {
      // Display each module
      if (qrcode_getModule(&qrcode, x2, y2)) {
        disp->fillRect(qr_x + x2 * 4, qr_y + y2 * 4, 4, 4, GxEPD_BLACK);
      }
    }
  }
  disp->fillRect(66, 66, 68, 68, GxEPD_WHITE);
  disp->drawXBitmap(68, 68, hb_logo_64x64, 64, 64, GxEPD_BLACK);

  disp->fillRect(34, 186, 132, 14, GxEPD_WHITE);
  u8g2.setFont(u8g2_font_profont17_tr);
  const char *text = device_state_.factory().serial_number.c_str();
  uint16_t w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 198);
  u8g2.print(text);

#elif defined(HOME_BUTTONS_PRO)
  uint16_t w;
  const char *text = "Home Buttons";
  u8g2.setFont(u8g2_font_helvB12_tr);
  w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 40);
  u8g2.print(text);

  disp->drawXBitmap(52, 52, hb_logo_24x24, 24, 24, GxEPD_BLACK);

  text = "------------------------";
  u8g2.setFont(u8g2_font_helvB12_tr);
  w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 102);
  u8g2.print(text);

  uint8_t version = 6;  // 41x41px
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(version)];
  qrcode_initText(&qrcode, qrcodeData, version, ECC_HIGH, DOCS_LINK);

  text = "Setup guide:";
  u8g2.setFont(u8g2_font_courR12_tr);
  w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 145);
  u8g2.print(text);

  uint16_t qr_x = 23;
  uint16_t qr_y = 165;
  for (uint8_t y2 = 0; y2 < qrcode.size; y2++) {
    // Each horizontal module
    for (uint8_t x2 = 0; x2 < qrcode.size; x2++) {
      // Display each module
      if (qrcode_getModule(&qrcode, x2, y2)) {
        disp->drawRect(qr_x + x2 * 2, qr_y + y2 * 2, 2, 2, GxEPD_BLACK);
      }
    }
  }

  u8g2.setFont(u8g2_font_profont12_tr);
  UIState::MessageType sw_ver = UIState::MessageType("SW: ") + SW_VERSION;
  u8g2.setCursor(0, 272);
  u8g2.print(sw_ver.c_str());

  UIState::MessageType model_info = UIState::MessageType("Model: ") +
                                    device_state_.factory().model_id.c_str() +
                                    " rev " +
                                    device_state_.factory().hw_version.c_str();
  u8g2.setCursor(0, 283);
  u8g2.print(model_info.c_str());

  u8g2.setCursor(0, 294);
  u8g2.print(device_state_.factory().unique_id.c_str());
#endif

  disp->display();
}

void Display::draw_settings() {
  disp->setRotation(ROTATION);
  disp->setFullWindow();

  u8g2.setFontMode(1);
  u8g2.setForegroundColor(text_color);
  u8g2.setBackgroundColor(bg_color);

  disp->fillScreen(bg_color);

#if defined(HOME_BUTTONS_ORIGINAL)
  disp->drawXBitmap(0, 17, account_cog_64x64, 64, 64, text_color);
  disp->drawXBitmap(WIDTH / 2, 17, wifi_cog_64x64, 64, 64, text_color);
  disp->drawXBitmap(0, 116, restore_64x64, 64, 64, text_color);
  disp->drawXBitmap(WIDTH / 2, 116, close_64x64, 64, 64, text_color);

  disp->drawXBitmap(40, 200, hb_logo_48x48, 48, 48, text_color);

  u8g2.setFont(u8g2_font_profont12_tr);

  u8g2.setCursor(0, 261);
  u8g2.print(device_state_.device_name().c_str());

  UIState::MessageType sw_ver = UIState::MessageType("SW: ") + SW_VERSION;
  u8g2.setCursor(0, 272);
  u8g2.print(sw_ver.c_str());

  UIState::MessageType model_info = UIState::MessageType("Model: ") +
                                    device_state_.factory().model_id.c_str() +
                                    " rev " +
                                    device_state_.factory().hw_version.c_str();
  u8g2.setCursor(0, 283);
  u8g2.print(model_info.c_str());

  u8g2.setCursor(0, 294);
  u8g2.print(device_state_.factory().unique_id.c_str());

#elif defined(HOME_BUTTONS_MINI)
  disp->drawXBitmap(0, 0, account_cog_100x100, 100, 100, text_color);
  disp->drawXBitmap(100, 0, wifi_cog_100x100, 100, 100, text_color);
  disp->drawXBitmap(0, 100, restore_100x100, 100, 100, text_color);
  disp->drawXBitmap(100, 100, close_100x100, 100, 100, text_color);

#elif defined(HOME_BUTTONS_PRO)
  uint16_t x_icon = 310;
  uint16_t x_text = 25;
  u8g2.setFont(u8g2_font_helvB18_te);
  int8_t ascent = u8g2.getFontAscent();

  disp->drawXBitmap(x_icon, 5, account_cog_64x64, 64, 64, text_color);
  u8g2.setCursor(x_text, 5 + 64 / 2 + ascent / 2);
  u8g2.print("Setup");
  disp->drawFastHLine(0, 74, WIDTH, text_color);

  disp->drawXBitmap(x_icon, 79, wifi_cog_64x64, 64, 64, text_color);
  u8g2.setCursor(x_text, 79 + 64 / 2 + ascent / 2);
  u8g2.print("Wi-Fi Setup");
  disp->drawFastHLine(0, 149, WIDTH, text_color);

  disp->drawXBitmap(x_icon, 154, restore_64x64, 64, 64, text_color);
  u8g2.setCursor(x_text, 154 + 64 / 2 + ascent / 2);
  u8g2.print("Restart");
  disp->drawFastHLine(0, 224, WIDTH, text_color);

  disp->drawXBitmap(x_icon, 229, close_64x64, 64, 64, text_color);
  u8g2.setCursor(x_text, 229 + 64 / 2 + ascent / 2);
  u8g2.print("Exit");

#endif

  disp->display();
}

void Display::draw_ap_config() {
  UIState::MessageType contents = UIState::MessageType("WIFI:T:WPA;S:") +
                                  device_state_.get_ap_ssid().c_str() +
                                  ";P:" + device_state_.get_ap_password() +
                                  ";;";

  disp->setRotation(ROTATION);
  disp->setFullWindow();

  u8g2.setFontMode(1);
  u8g2.setForegroundColor(text_color);
  u8g2.setBackgroundColor(bg_color);

  disp->fillScreen(bg_color);

#if defined(HOME_BUTTONS_ORIGINAL)
  uint8_t version = 6;  // 41x41px
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(version)];
  qrcode_initText(&qrcode, qrcodeData, version, ECC_HIGH, contents.c_str());

  u8g2.setFont(u8g2_font_courR12_tr);

  uint16_t w;
  const char *text = "Scan:";
  w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 20);
  u8g2.print(text);

  uint16_t qr_x = 23;
  uint16_t qr_y = 35;
  for (uint8_t y2 = 0; y2 < qrcode.size; y2++) {
    // Each horizontal module
    for (uint8_t x2 = 0; x2 < qrcode.size; x2++) {
      // Display each module
      if (qrcode_getModule(&qrcode, x2, y2)) {
        disp->drawRect(qr_x + x2 * 2, qr_y + y2 * 2, 2, 2, GxEPD_BLACK);
      }
    }
  }
  text = "--------- or ---------";
  u8g2.setFont(u8g2_font_helvB12_tr);
  w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 153);
  u8g2.print(text);

  text = "Connect to:";
  u8g2.setFont(u8g2_font_courR12_tr);
  w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 190);
  u8g2.print(text);

  u8g2.setCursor(0, 220);
  u8g2.print("Wi-Fi:");
  u8g2.setFont(u8g2_font_helvB12_tr);
  u8g2.setCursor(0, 235);
  u8g2.print(device_state_.get_ap_ssid().c_str());

  u8g2.setFont(u8g2_font_courR12_tr);
  u8g2.setCursor(0, 260);
  u8g2.print("Password:");
  u8g2.setFont(u8g2_font_helvB12_tr);
  u8g2.setCursor(0, 275);
  u8g2.print(device_state_.get_ap_password());

#elif defined(HOME_BUTTONS_MINI)
  uint8_t version = 8;  // 49x49px
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(version)];
  qrcode_initText(&qrcode, qrcodeData, version, ECC_HIGH, contents.c_str());
  uint16_t qr_x = 2;
  uint16_t qr_y = 2;
  for (uint8_t y2 = 0; y2 < qrcode.size; y2++) {
    // Each horizontal module
    for (uint8_t x2 = 0; x2 < qrcode.size; x2++) {
      // Display each module
      if (qrcode_getModule(&qrcode, x2, y2)) {
        disp->fillRect(qr_x + x2 * 4, qr_y + y2 * 4, 4, 4, GxEPD_BLACK);
      }
    }
  }
  disp->fillRect(66, 66, 68, 68, GxEPD_WHITE);
  disp->drawXBitmap(68, 68, wifi_cog_64x64, 64, 64, GxEPD_BLACK);

  disp->fillRect(34, 186, 132, 14, GxEPD_WHITE);
  u8g2.setFont(u8g2_font_profont17_tr);
  const char *text = device_state_.get_ap_ssid().c_str();
  uint16_t w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 198);
  u8g2.print(text);

#elif defined(HOME_BUTTONS_PRO)
  uint8_t version = 6;  // 41x41px
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(version)];
  qrcode_initText(&qrcode, qrcodeData, version, ECC_HIGH, contents.c_str());

  u8g2.setFont(u8g2_font_courR12_tr);

  uint16_t w;
  const char *text = "Scan:";
  w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 20);
  u8g2.print(text);

  uint16_t qr_x = 23;
  uint16_t qr_y = 35;
  for (uint8_t y2 = 0; y2 < qrcode.size; y2++) {
    // Each horizontal module
    for (uint8_t x2 = 0; x2 < qrcode.size; x2++) {
      // Display each module
      if (qrcode_getModule(&qrcode, x2, y2)) {
        disp->drawRect(qr_x + x2 * 2, qr_y + y2 * 2, 2, 2, GxEPD_BLACK);
      }
    }
  }
  text = "--------- or ---------";
  u8g2.setFont(u8g2_font_helvB12_tr);
  w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 153);
  u8g2.print(text);

  text = "Connect to:";
  u8g2.setFont(u8g2_font_courR12_tr);
  w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 190);
  u8g2.print(text);

  u8g2.setCursor(0, 220);
  u8g2.print("Wi-Fi:");
  u8g2.setFont(u8g2_font_helvB12_tr);
  u8g2.setCursor(0, 235);
  u8g2.print(device_state_.get_ap_ssid().c_str());

  u8g2.setFont(u8g2_font_courR12_tr);
  u8g2.setCursor(0, 260);
  u8g2.print("Password:");
  u8g2.setFont(u8g2_font_helvB12_tr);
  u8g2.setCursor(0, 275);
  u8g2.print(device_state_.get_ap_password());
#endif

  disp->display();
}

void Display::draw_web_config() {
  UIState::MessageType contents =
      UIState::MessageType("http://") + device_state_.ip();

  disp->setRotation(ROTATION);
  disp->setFullWindow();

  u8g2.setFontMode(1);
  u8g2.setForegroundColor(text_color);
  u8g2.setBackgroundColor(bg_color);

  disp->fillScreen(bg_color);

#if defined(HOME_BUTTONS_ORIGINAL)
  uint8_t version = 6;  // 41x41px
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(version)];
  qrcode_initText(&qrcode, qrcodeData, version, ECC_HIGH, contents.c_str());

  u8g2.setFont(u8g2_font_courR12_tr);

  uint16_t w;
  const char *text = "Scan:";
  w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 20);
  u8g2.print(text);

  uint16_t qr_x = 23;
  uint16_t qr_y = 35;

  for (uint8_t y2 = 0; y2 < qrcode.size; y2++) {
    // Each horizontal module
    for (uint8_t x2 = 0; x2 < qrcode.size; x2++) {
      // Display each module
      if (qrcode_getModule(&qrcode, x2, y2)) {
        disp->drawRect(qr_x + x2 * 2, qr_y + y2 * 2, 2, 2, GxEPD_BLACK);
      }
    }
  }
  text = "--------- or ---------";
  u8g2.setFont(u8g2_font_helvB12_tr);
  w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 153);
  u8g2.print(text);

  text = "Go to:";
  u8g2.setFont(u8g2_font_courR12_tr);
  w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 200);
  u8g2.print(text);

  u8g2.setFont(u8g2_font_helvB12_tr);
  u8g2.setCursor(0, 240);
  u8g2.print("http://");
  u8g2.setCursor(0, 260);
  u8g2.print(device_state_.ip());

#elif defined(HOME_BUTTONS_MINI)
  uint8_t version = 8;  // 49x49px
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(version)];
  qrcode_initText(&qrcode, qrcodeData, version, ECC_HIGH, contents.c_str());
  uint16_t qr_x = 2;
  uint16_t qr_y = 2;
  for (uint8_t y2 = 0; y2 < qrcode.size; y2++) {
    // Each horizontal module
    for (uint8_t x2 = 0; x2 < qrcode.size; x2++) {
      // Display each module
      if (qrcode_getModule(&qrcode, x2, y2)) {
        disp->fillRect(qr_x + x2 * 4, qr_y + y2 * 4, 4, 4, GxEPD_BLACK);
      }
    }
  }
  disp->fillRect(66, 66, 68, 68, GxEPD_WHITE);
  disp->drawXBitmap(68, 68, account_cog_64x64, 64, 64, GxEPD_BLACK);

  disp->fillRect(34, 186, 132, 14, GxEPD_WHITE);
  u8g2.setFont(u8g2_font_profont17_tr);
  const char *text = device_state_.ip();
  uint16_t w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 198);
  u8g2.print(text);

#elif defined(HOME_BUTTONS_PRO)
  uint8_t version = 6;  // 41x41px
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(version)];
  qrcode_initText(&qrcode, qrcodeData, version, ECC_HIGH, contents.c_str());

  u8g2.setFont(u8g2_font_courR12_tr);

  uint16_t w;
  const char *text = "Scan:";
  w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 20);
  u8g2.print(text);

  uint16_t qr_x = 23;
  uint16_t qr_y = 35;

  for (uint8_t y2 = 0; y2 < qrcode.size; y2++) {
    // Each horizontal module
    for (uint8_t x2 = 0; x2 < qrcode.size; x2++) {
      // Display each module
      if (qrcode_getModule(&qrcode, x2, y2)) {
        disp->drawRect(qr_x + x2 * 2, qr_y + y2 * 2, 2, 2, GxEPD_BLACK);
      }
    }
  }
  text = "--------- or ---------";
  u8g2.setFont(u8g2_font_helvB12_tr);
  w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 153);
  u8g2.print(text);

  text = "Go to:";
  u8g2.setFont(u8g2_font_courR12_tr);
  w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 200);
  u8g2.print(text);

  u8g2.setFont(u8g2_font_helvB12_tr);
  u8g2.setCursor(0, 240);
  u8g2.print("http://");
  u8g2.setCursor(0, 260);
  u8g2.print(device_state_.ip());
#endif

  disp->display();
}

void Display::draw_test(const char *text, const char *mdi_name,
                        uint16_t mdi_size) {
  uint16_t fg, bg;
  fg = GxEPD_BLACK;
  bg = GxEPD_WHITE;

  disp->setRotation(ROTATION);
  disp->setFullWindow();

  u8g2.setFontMode(1);
  u8g2.setForegroundColor(fg);
  u8g2.setBackgroundColor(bg);

  disp->fillScreen(bg);

#if defined(HOME_BUTTONS_ORIGINAL)
  mdi_.begin();
  draw_mdi(mdi_name, mdi_size, WIDTH / 2 - mdi_size / 2, 50);
  mdi_.end();

  u8g2.setFont(u8g2_font_helvB24_te);
  uint16_t w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 250);
  u8g2.print(text);
#elif defined(HOME_BUTTONS_MINI)
  mdi_.begin();
  draw_mdi(mdi_name, mdi_size, WIDTH / 2 - mdi_size / 2, 20);
  mdi_.end();

  u8g2.setFont(u8g2_font_helvB24_te);
  uint16_t w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 175);
  u8g2.print(text);
#elif defined(HOME_BUTTONS_PRO)
  u8g2.setFont(u8g2_font_helvB24_te);
  uint16_t w = u8g2.getUTF8Width(text);
  u8g2.setCursor(WIDTH / 2 - w / 2, 250);
  u8g2.print(text);
#endif

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

// based on GxEPD2_Spiffs_Example.ino - drawBitmapFromSpiffs_Buffered()
// Warning - SPIFFS.begin() must be called before this function
bool Display::draw_bmp(File &file, int16_t x, int16_t y) {
  uint32_t startTime = millis();
  if (!file) {
    error("error opening file");
    return false;
  }
  bool valid = false;  // valid format to be handled
  bool flip = true;    // bitmap is stored bottom-to-top
  if ((x >= disp->width()) || (y >= disp->height())) return false;

  // Parse BMP header
  if (read16(file) == 0x4D42) {
    debug("BMP signature detected");
    uint32_t fileSize = read32(file);
    uint32_t creatorBytes = read32(file);
    (void)creatorBytes;                   // unused
    uint32_t imageOffset = read32(file);  // Start of image data
    uint32_t headerSize = read32(file);
    uint32_t width = read32(file);
    int32_t height = (int32_t)read32(file);
    uint16_t planes = read16(file);
    uint16_t depth = read16(file);  // bits per pixel
    uint32_t format = read32(file);
    if ((planes == 1) && ((format == 0) || (format == 3))) {
      debug("BMP Image Offset: %d", imageOffset);
      debug("BMP Header size: %d", headerSize);
      debug("BMP File size: %d", fileSize);
      debug("BMP Bit Depth: %d", depth);
      debug("BMP Image size: %d x %d", width, height);
      // BMP rows are padded (if needed) to 4-byte boundary
      uint32_t rowSize = (width * depth / 8 + 3) & ~3;
      if (depth < 8) rowSize = ((width * depth + 8 - depth) / 8 + 3) & ~3;
      if (height < 0) {
        height = -height;
        flip = false;
      }
      uint16_t w = width;
      uint16_t h = height;
      if ((x + w - 1) >= disp->width()) w = disp->width() - x;
      if ((y + h - 1) >= disp->height()) h = disp->height() - y;
      valid = true;
      uint8_t bitmask = 0xFF;
      uint8_t bitshift = 8 - depth;
      uint16_t red, green, blue;
      bool whitish = false;
      bool colored = false;
      if (depth <= 8) {
        if (depth < 8) bitmask >>= depth;
        file.seek(imageOffset - (4 << depth));
        for (uint16_t pn = 0; pn < (1 << depth); pn++) {
          blue = file.read();
          green = file.read();
          red = file.read();
          file.read();
          whitish = (red + green + blue) > 3 * 0x80;
          // reddish or yellowish?
          colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));
          if (0 == pn % 8) mono_palette_buffer[pn / 8] = 0;
          mono_palette_buffer[pn / 8] |= whitish << pn % 8;
          if (0 == pn % 8) color_palette_buffer[pn / 8] = 0;
          color_palette_buffer[pn / 8] |= colored << pn % 8;
          rgb_palette_buffer[pn] = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) |
                                   ((blue & 0xF8) >> 3);
        }
      }
      uint32_t rowPosition =
          flip ? imageOffset + (height - h) * rowSize : imageOffset;
      for (uint16_t row = 0; row < h;
           row++, rowPosition += rowSize)  // for each line
      {
        uint32_t in_remain = rowSize;
        uint32_t in_idx = 0;
        uint32_t in_bytes = 0;
        uint8_t in_byte = 0;  // for depth <= 8
        uint8_t in_bits = 0;  // for depth <= 8
        uint16_t color = GxEPD_WHITE;
        file.seek(rowPosition);
        for (uint16_t col = 0; col < w; col++)  // for each pixel
        {
          // Time to read more pixel data?
          if (in_idx >= in_bytes)  // ok, exact match for 24bit also (size
                                   // IS multiple of 3)
          {
            in_bytes = file.read(input_buffer, in_remain > sizeof(input_buffer)
                                                   ? sizeof(input_buffer)
                                                   : in_remain);
            in_remain -= in_bytes;
            in_idx = 0;
          }
          switch (depth) {
            case 24:
              blue = input_buffer[in_idx++];
              green = input_buffer[in_idx++];
              red = input_buffer[in_idx++];
              whitish = (red + green + blue) > 3 * 0x80;
              // reddish or yellowish?
              colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));
              color = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) |
                      ((blue & 0xF8) >> 3);
              break;
            case 16: {
              uint8_t lsb = input_buffer[in_idx++];
              uint8_t msb = input_buffer[in_idx++];
              if (format == 0)  // 555
              {
                blue = (lsb & 0x1F) << 3;
                green = ((msb & 0x03) << 6) | ((lsb & 0xE0) >> 2);
                red = (msb & 0x7C) << 1;
                color = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) |
                        ((blue & 0xF8) >> 3);
              } else  // 565
              {
                blue = (lsb & 0x1F) << 3;
                green = ((msb & 0x07) << 5) | ((lsb & 0xE0) >> 3);
                red = (msb & 0xF8);
                color = (msb << 8) | lsb;
              }
              whitish = (red + green + blue) > 3 * 0x80;
              // reddish or yellowish?
              colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));
            } break;
            case 1:
            case 4:
            case 8: {
              if (0 == in_bits) {
                in_byte = input_buffer[in_idx++];
                in_bits = 8;
              }
              uint16_t pn = (in_byte >> bitshift) & bitmask;
              whitish = mono_palette_buffer[pn / 8] & (0x1 << pn % 8);
              colored = color_palette_buffer[pn / 8] & (0x1 << pn % 8);
              in_byte <<= depth;
              in_bits -= depth;
              color = rgb_palette_buffer[pn];
            } break;
          }
          if (whitish) {
            color = GxEPD_WHITE;
          } else if (colored) {
            color = GxEPD_COLORED;
          } else {
            color = GxEPD_BLACK;
          }
          uint16_t yrow = y + (flip ? h - row - 1 : row);
          disp->drawPixel(x + col, yrow, color);
        }  // end pixel
      }  // end line
    }
  }
  file.close();
  if (!valid) {
    error("BMP format not valid.");
  }
  debug("BMP loaded in %lu ms", millis() - startTime);
  return valid;
}

void Display::draw_mdi(const char *name, uint16_t size, int16_t x, int16_t y) {
  bool draw_placeholder = false;
  File file;
  if (mdi_.exists(name, size)) {
    File file = mdi_.get_file(name, size);
    if (!draw_bmp(file, x, y)) {
      error("Could not draw icon: %s", name);
      // file might be corrupted - remove so it will be downloaded again
      mdi_.remove(name, size);
      draw_placeholder = true;
    }
  } else {
    error("Icon not found: %s", name);
    draw_placeholder = true;
  }
  if (draw_placeholder) {
    if (size == 64) {
      disp->drawXBitmap(x, y, file_question_outline_64x64, 64, 64, text_color);
    } else if (size == 48) {
      disp->drawXBitmap(x, y, file_question_outline_48x48, 48, 48, text_color);
    } else if (size == 100) {
      disp->drawXBitmap(x, y, file_question_outline_100x100, 100, 100,
                        text_color);
    } else if (size == 92) {
      disp->drawXBitmap(x, y, file_question_outline_92x92, 92, 92, text_color);
    }
  }
  // disp->drawRect(x, y, size, size, text_color);
}

void ButtonTile::draw(Display &display, int16_t x, int16_t y, uint16_t color) {
  // display.disp->drawRect(x, y, width, height, color);
  switch (label_type) {
    case LabelType::Icon: {
      uint16_t mdi_size = 92;
      if (width < mdi_size || height < mdi_size) {
        display.error("ButtonTile::draw: Tile height too low");
        return;
      }
      int16_t icon_x = x + (width - mdi_size) / 2;
      int16_t icon_y = y + (height - mdi_size) / 2;
      display.mdi_.begin();
      display.draw_mdi(mdi_name.c_str(), mdi_size, icon_x, icon_y);
      display.mdi_.end();
      break;
    }
    case LabelType::Mixed: {
      uint16_t mdi_size = 64;

      display.u8g2.setFont(u8g2_font_helvB18_te);
      uint16_t h_padding = 4;
      uint16_t v_padding = 4;
      int8_t ascent = display.u8g2.getFontAscent();
      int8_t descent = display.u8g2.getFontDescent();
      display.debug("Font ascent: %d, descent: %d", ascent, descent);
      if (mdi_size + ascent - descent + v_padding > height) {
        display.error("ButtonTile::draw: Tile height too low");
        return;
      }
      int16_t icon_x = x + (width - mdi_size) / 2;
      int16_t icon_y = y + v_padding;
      display.mdi_.begin();
      display.draw_mdi(mdi_name.c_str(), mdi_size, icon_x, icon_y);
      display.mdi_.end();

      text = display.trim_text(text, width - 2 * h_padding);
      uint16_t text_width = display.u8g2.getUTF8Width(text.c_str());
      display.u8g2.setCursor(x + width / 2 - text_width / 2,
                             y + height - v_padding + descent);
      display.u8g2.print(text.c_str());
      break;
    }
    case LabelType::Text: {
      uint16_t h_padding = 4;
      display.u8g2.setFont(u8g2_font_helvB24_te);
      uint16_t text_width = display.u8g2.getUTF8Width(text.c_str());
      if (text_width >= width - 2 * h_padding) {
        display.u8g2.setFont(u8g2_font_helvB18_te);
        text = display.trim_text(text, width - 2 * h_padding);
      }
      text_width = display.u8g2.getUTF8Width(text.c_str());
      int8_t ascent = display.u8g2.getFontAscent();
      display.u8g2.setCursor(x + width / 2 - text_width / 2,
                             y + height / 2 + ascent / 2);
      display.u8g2.print(text.c_str());
      break;
    }
    default:
      break;
  }
}
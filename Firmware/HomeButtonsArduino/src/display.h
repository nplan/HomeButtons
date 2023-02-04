#ifndef HOMEBUTTONS_DISPLAY_H
#define HOMEBUTTONS_DISPLAY_H

#include "state.h"
#include <GxEPD2.h>

enum class DisplayPage {
    EMPTY,
    MAIN,
    INFO,
    MESSAGE,
    MESSAGE_LARGE,
    ERROR,
    WELCOME,
    AP_CONFIG,
    WEB_CONFIG,
    TEST,
    TEST_INV
};

struct UIState {
  DisplayPage page = DisplayPage::EMPTY;
  String message = "";
  bool disappearing = false;
  uint32_t appear_time = 0;
  uint32_t disappear_timeout = 0;
};

class Display {
 public:

  enum class State {
    IDLE,
    ACTIVE,
    CMD_END,
    ENDING
  };

  void begin();
  void end();
  void update();

  void disp_message(String message, uint32_t duration = 0);
  void disp_message_large(String message, uint32_t duration = 0);
  void disp_error(String message, uint32_t duration = 0);
  void disp_main();
  void disp_info();
  void disp_welcome();
  void disp_ap_config();
  void disp_web_config();
  void disp_test(bool invert = false);

  UIState get_ui_state();
  void init_ui_state(UIState ui_state); // used after wakeup
  State get_state();

 private:
  State state = State::IDLE;

  UIState current_ui_state = {};
  UIState cmd_ui_state = {};
  UIState draw_ui_state = {};
  UIState pre_disappear_ui_state = {};
  
  bool new_ui_cmd = false;
  bool redraw_in_progress = false;
  
  uint16_t text_color = GxEPD_BLACK;
  uint16_t bg_color = GxEPD_WHITE;

  void set_cmd_state(UIState cmd);

  void draw_message(String message, bool error = false, bool large = false);
  void draw_main();
  void draw_info();
  void draw_welcome();
  void draw_ap_config();
  void draw_web_config();
  void draw_test(bool invert = false);
  void draw_white();
  void draw_black();
};

extern Display display;

#endif // HOMEBUTTONS_DISPLAY_H
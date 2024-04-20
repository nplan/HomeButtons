#ifndef DISPLAY_DRAW_H
#define DISPLAY_DRAW_H

#include <stdint.h>
#include <FS.h>
#include "types.h"

class DisplayDraw {
 public:
  virtual void draw_main() = 0;
  virtual void draw_info() = 0;
  virtual void draw_device_info() = 0;
  virtual void draw_welcome() = 0;
  virtual void draw_settings() = 0;
  virtual void draw_ap_config() = 0;
  virtual void draw_web_config() = 0;
  virtual void draw_test(const char* text, const char* mdi_name,
                         uint16_t mdi_size) = 0;

  virtual void draw_message(const UIState::MessageType& message,
                            bool error = false, bool large = false) = 0;
  virtual void draw_white() = 0;
  virtual void draw_black() = 0;
  virtual bool draw_bmp(File& file, int16_t x, int16_t y) = 0;
  virtual void draw_mdi(const char* name, uint16_t size, int16_t x,
                        int16_t y) = 0;
};

#endif  // DISPLAY_DRAW_H

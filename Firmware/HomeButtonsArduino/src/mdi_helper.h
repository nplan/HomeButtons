#ifndef HOMEBUTTONS_MDI_HELPER_H
#define HOMEBUTTONS_MDI_HELPER_H

#include <SPIFFS.h>

#include "logger.h"
#include "static_string.h"

static constexpr uint8_t MAX_NUM_SIZES = 3;

class MDIHelper : public Logger {
 public:
  MDIHelper() : Logger("MDI") {}
  bool begin();
  void add_size(uint16_t size);
  bool download(const char* name, uint16_t size);
  bool download(const char* name);
  bool check_connection();
  bool exists(const char* name, uint16_t size);
  bool exists(const char* name);
  File get_file(const char* name, uint16_t size);
  size_t get_free_space();
  bool make_space(size_t size);
  bool remove(const char* name, uint16_t size);
  void end();

 private:
  bool spiffs_mounted_ = false;
  uint16_t sizes_[MAX_NUM_SIZES] = {0};
  uint8_t num_sizes_ = 0;
  void _get_path(char* out, const char* name, uint16_t size);
};

#endif

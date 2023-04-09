#ifndef HOMEBUTTONS_MDI_HELPER_H
#define HOMEBUTTONS_MDI_HELPER_H

#include <SPIFFS.h>

#include "logger.h"
#include "static_string.h"

class MDIHelper : public Logger {
 public:
  MDIHelper() : Logger("MDI") {}
  bool begin();
  void get_path(char* out, const char* name);
  bool download(const char* name);
  bool check_connection();
  bool exists(const char* name);
  File get_file(const char* name);
  size_t get_free_space();
  bool make_space(size_t size = 10000UL);
  bool remove(const char* name);
  void end();

 private:
  bool spiffs_mounted_ = false;
};

#endif

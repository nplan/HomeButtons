#ifndef HOMEBUTTONS_LOGGER_H
#define HOMEBUTTONS_LOGGER_H

#include "static_string.h"
#include "esp_log.h"

class Logger {
 public:
  Logger(const char* tag) : tag_(tag) {}

  static constexpr std::size_t MAX_LOG_LINE_SIZE = 128;

  void __attribute__((format(printf, 2, 3))) debug(const char* fmt, ...) const {
    char buffer[MAX_LOG_LINE_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    ESP_LOGD(tag_.c_str(), "%s", buffer);
  }

  void __attribute__((format(printf, 2, 3))) info(const char* fmt, ...) const {
    char buffer[MAX_LOG_LINE_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    ESP_LOGI(tag_.c_str(), "%s", buffer);
  }

  void __attribute__((format(printf, 2, 3)))
  warning(const char* fmt, ...) const {
    char buffer[MAX_LOG_LINE_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    ESP_LOGW(tag_.c_str(), "%s", buffer);
  }

  void __attribute__((format(printf, 2, 3))) error(const char* fmt, ...) const {
    char buffer[MAX_LOG_LINE_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    ESP_LOGE(tag_.c_str(), "%s", buffer);
  }

 private:
  static constexpr std::size_t MAX_TAG_LENGTH = 16;
  StaticString<MAX_TAG_LENGTH> tag_;
};

#endif  // HOMEBUTTONS_LOGGER_H

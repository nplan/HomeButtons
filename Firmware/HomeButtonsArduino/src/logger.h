#ifndef HOMEBUTTONS_LOGGER_H
#define HOMEBUTTONS_LOGGER_H

#include "static_string.h"
#include "esp_log.h"

#ifndef LOGGER_DEFAULT_LOG_LEVEL
#define LOGGER_DEFAULT_LOG_LEVEL ESP_LOG_INFO
#endif

class Logger {
 public:
  Logger(const char* tag, esp_log_level_t level = LOGGER_DEFAULT_LOG_LEVEL)
      : tag_(tag), log_level_(level) {
    _computed_padded_tag();
    esp_log_level_set(tag_.c_str(), level);
    ESP_LOGD("logger", "Log level for %s: %c", tag, _log_level_to_char(level));
  }

  static constexpr std::size_t MAX_LOG_LINE_SIZE = 128;

  void __attribute__((format(printf, 2, 3))) debug(const char* fmt, ...) const {
    va_list args;
    va_start(args, fmt);
    _log(ESP_LOG_DEBUG, fmt, args);
    va_end(args);
  }

  void __attribute__((format(printf, 2, 3))) info(const char* fmt, ...) const {
    va_list args;
    va_start(args, fmt);
    _log(ESP_LOG_INFO, fmt, args);
    va_end(args);
  }

  void __attribute__((format(printf, 2, 3)))
  warning(const char* fmt, ...) const {
    va_list args;
    va_start(args, fmt);
    _log(ESP_LOG_WARN, fmt, args);
    va_end(args);
  }

  void __attribute__((format(printf, 2, 3))) error(const char* fmt, ...) const {
    va_list args;
    va_start(args, fmt);
    _log(ESP_LOG_ERROR, fmt, args);
    va_end(args);
  }

 private:
  static constexpr std::size_t MAX_TAG_LENGTH = 8;
  StaticString<MAX_TAG_LENGTH> tag_;
  StaticString<MAX_TAG_LENGTH> padded_tag_;
  esp_log_level_t log_level_;

  constexpr char _log_level_to_char(esp_log_level_t level) const {
    switch (level) {
      case ESP_LOG_ERROR:
        return 'E';
      case ESP_LOG_WARN:
        return 'W';
      case ESP_LOG_INFO:
        return 'I';
      case ESP_LOG_DEBUG:
        return 'D';
      default:
        return '?';
    }
  }

  constexpr const char* _log_level_to_log_color(esp_log_level_t level) const {
    switch (level) {
      case ESP_LOG_ERROR:
        return LOG_COLOR_E "";
      case ESP_LOG_WARN:
        return LOG_COLOR_W "";
      case ESP_LOG_INFO:
        return LOG_COLOR_I "";
      case ESP_LOG_DEBUG:
        return LOG_COLOR_D "";
      default:
        return "";
    }
  }

  void _computed_padded_tag() {
    padded_tag_ = tag_;
    while (padded_tag_.length() < MAX_TAG_LENGTH) {
      padded_tag_ += ' ';
    }
  }

  // va_list args
  void _log(esp_log_level_t level, const char* fmt, va_list args) const {
    if (level > log_level_) return;
    char buffer[MAX_LOG_LINE_SIZE];
    int rc =
        snprintf(buffer, MAX_LOG_LINE_SIZE, "%s[%6u][%c][%s] " LOG_RESET_COLOR,
                 _log_level_to_log_color(level), esp_log_timestamp(),
                 _log_level_to_char(level), padded_tag_.c_str());

    if (rc < 0) {
      ESP_LOGE("logger", "fatal: unable to log");
      return;
    }

    if (rc < MAX_LOG_LINE_SIZE) {
      vsnprintf(&buffer[rc], MAX_LOG_LINE_SIZE - rc, fmt, args);
      esp_log_write(level, tag_.c_str(), "%s\n", buffer);
    } else {
      ESP_LOGE("logger", "fatal: unable to log");
    }
  }
};

#endif  // HOMEBUTTONS_LOGGER_H

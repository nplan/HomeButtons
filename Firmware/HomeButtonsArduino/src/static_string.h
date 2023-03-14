#ifndef HOMEBUTTONS_STATICSTRING_H
#define HOMEBUTTONS_STATICSTRING_H

#include <cstdio>
#include <cstring>
#include <algorithm>
#include "Arduino.h"

// A class to hold a string up to MAX_SIZE characters (or MAX_SIZE+1 characters
// including \0)
template <size_t _MAX_SIZE>
class StaticString {
  static constexpr size_t MAX_SIZE = _MAX_SIZE + 1;  // to include trailing '\0'

 public:
  StaticString() {}
  explicit StaticString(const char* str) {
    auto n = std::snprintf(data_, MAX_SIZE, "%s", str);
    _check_snprintf_return_value(n);
  }

  explicit StaticString(const String& str) {
    auto n = std::snprintf(data_, MAX_SIZE, "%s", str.c_str());
    _check_snprintf_return_value(n);
  }

  template <size_t _OTHER_MAX_SIZE>
  StaticString(const StaticString<_OTHER_MAX_SIZE>& other) {
    auto n = std::snprintf(data_, MAX_SIZE, "%s", other.c_str());
    _check_snprintf_return_value(n);
  }

  template <typename... Args>
  explicit StaticString(const char* format, Args... args) {
    auto n = std::snprintf(data_, MAX_SIZE, format, args...);
    _check_snprintf_return_value(n);
  }

  template <typename... Args>
  void set(Args... args) {
    *this = StaticString(args...);
  }

  const char* c_str() const { return data_; }

  size_t length() const { return strlen(data_); }
  bool empty() const { return strlen(data_) == 0; }
  StaticString substring(size_t i, size_t j) {
    StaticString output;
    if (i < MAX_SIZE) {
      auto n =
          std::snprintf(output.data_, std::min(MAX_SIZE, j - i + 1), data_ + i);
      _check_snprintf_return_value(n);
    }
    return output;
  }

  template <typename T>
  StaticString operator+(T other) const {
    StaticString output = *this;
    output += other;
    return output;
  }

  template <size_t _OTHER_MAX_SIZE>
  StaticString& operator+=(const StaticString<_OTHER_MAX_SIZE>& other) {
    *this += other.c_str();
    return *this;
  }

  StaticString& operator+=(const String& other) {
    *this += other.c_str();
    return *this;
  }

  StaticString& operator+=(unsigned long i) {
    auto offset = length();
    auto n = std::snprintf(&data_[offset], MAX_SIZE - offset, "%lu", i);
    _check_snprintf_return_value(n);
    return *this;
  }

  StaticString& operator+=(int i) {
    auto offset = length();
    auto n = std::snprintf(&data_[offset], MAX_SIZE - offset, "%d", i);
    _check_snprintf_return_value(n);
    return *this;
  }

  StaticString& operator+=(const char* other) {
    auto offset = length();
    auto n = std::snprintf(&data_[offset], MAX_SIZE - offset, "%s", other);
    _check_snprintf_return_value(n);
    return *this;
  }

  StaticString& operator+=(char other) {
    auto offset = length();
    auto n = std::snprintf(&data_[offset], MAX_SIZE - offset, "%c", other);
    _check_snprintf_return_value(n);
    return *this;
  }

  StaticString& operator=(const char* other) {
    auto n = std::snprintf(data_, MAX_SIZE, "%s", other);
    _check_snprintf_return_value(n);
    return *this;
  }

  bool operator==(const char* other) const {
    return std::strncmp(data_, other, MAX_SIZE) == 0;
  }

  template <size_t _OTHER_MAX_SIZE>
  bool operator==(const StaticString<_OTHER_MAX_SIZE> other) const {
    return std::strncmp(data_, other.data_,
                        std::min(MAX_SIZE,
                                 StaticString<_OTHER_MAX_SIZE>::MAX_SIZE)) == 0;
  }

 private:
  char data_[MAX_SIZE] = {'\0'};
  void _check_snprintf_return_value(int value) const {
    if (value >= MAX_SIZE)
      ESP_LOGE("static_string",
               "buffer too small (size: %d, wanted: %d, content: %s)", MAX_SIZE,
               value, data_);
    else if (value < 0)
      ESP_LOGE("static_string", "format failed");
  }
};

#endif  // HOMEBUTTONS_STATICSTRING_H

#ifndef HOMEBUTTONS_STATICSTRING_H
#define HOMEBUTTONS_STATICSTRING_H

#include <cstdio>
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
    auto n = std::snprintf(m_data, MAX_SIZE, "%s", str);
    _check_snprintf_return_value(n);
  }

  template <typename... Args>
  explicit StaticString(const char* format, Args... args) {
    auto n = std::snprintf(m_data, MAX_SIZE, format, args...);
    _check_snprintf_return_value(n);
  }

  template <typename... Args>
  void set(Args... args) {
    *this = StaticString(args...);
  }

  const char* c_str() const { return m_data; }

  size_t length() const { return strlen(m_data); }
  bool empty() const { return strlen(m_data) == 0; }
  StaticString substring(size_t i, size_t j) {
    StaticString output;
    if (i < MAX_SIZE) {
      auto n = std::snprintf(output.m_data, std::min(MAX_SIZE, j - i + 1),
                             m_data + i);
      _check_snprintf_return_value(n);
    }
    return output;
  }

  StaticString operator+(const StaticString& other) {
    StaticString output;
    auto n = snprintf(output.m_data, MAX_SIZE, "%s%s", m_data, other.m_data);
    _check_snprintf_return_value(n);
    return output;
  }

  StaticString operator+(const char* other) {
    StaticString output;
    auto n = std::snprintf(output.m_data, MAX_SIZE, "%s%s", m_data, other);
    _check_snprintf_return_value(n);
    return output;
  }

  StaticString operator+(unsigned long i) {
    StaticString output;
    auto n = std::snprintf(output.m_data, MAX_SIZE, "%s%lu", m_data, i);
    _check_snprintf_return_value(n);
    return output;
  }

  StaticString& operator=(const char* other) {
    auto n = std::snprintf(m_data, MAX_SIZE, "%s", other);
    _check_snprintf_return_value(n);
    return *this;
  }

 private:
  char m_data[MAX_SIZE] = {'\0'};
  void _check_snprintf_return_value(int value) {
    if (value >= MAX_SIZE)
      log_w("warning, StaticString too small");
    else if (value < 0)
      log_w("warning, StaticString format failed");
  }
};

#endif  // HOMEBUTTONS_STATICSTRING_H
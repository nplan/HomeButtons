#ifndef HOMEBUTTONS_STATICSTRING_H
#define HOMEBUTTONS_STATICSTRING_H

#include <cstdlib>

// A class to hold a string up to MAX_SIZE characters (or MAX_SIZE+1 characters
// including \0)
template <size_t _MAX_SIZE>
class StaticString {
  static constexpr size_t MAX_SIZE = _MAX_SIZE + 1;  // to include trailing '\0'

 public:
  StaticString() {}
  explicit StaticString(const char* str) {
    snprintf(m_data, MAX_SIZE, "%s", str);
  }

  template <typename... Args>
  explicit StaticString(const char* format, Args... args) {
    auto n = std::snprintf(m_data, MAX_SIZE, format, args...);
    if (n >= MAX_SIZE) log_d("warning, StaticString too small");
  }

  template <typename... Args>
  void set(Args... args) {
    *this = StaticString(args...);
  }

  const char* c_str() const { return m_data; }

  size_t length() const { return strlen(m_data); }
  StaticString substring(size_t i, size_t j) {
    StaticString output;
    if (i < MAX_SIZE) {
      snprintf(output.m_data, std::min(MAX_SIZE, j - i + 1), m_data + i);
    }
    return output;
  }

  StaticString operator+(const StaticString& other) {
    StaticString output;
    auto n = snprintf(output.m_data, MAX_SIZE, "%s%s", m_data, other.m_data);
    if (n >= MAX_SIZE) log_d("warning, StaticString too small");
    return output;
  }

  StaticString operator+(const char* other) {
    StaticString output;
    auto n = std::snprintf(output.m_data, MAX_SIZE, "%s%s", m_data, other);
    if (n >= MAX_SIZE) log_d("warning, StaticString too small");
    return output;
  }

  StaticString& operator=(const char* other) {
    auto n = std::snprintf(m_data, MAX_SIZE, "%s", other);
    if (n >= MAX_SIZE) log_d("warning, StaticString too small");
    return *this;
  }

 private:
  char m_data[MAX_SIZE] = {'\0'};
};

#endif  // HOMEBUTTONS_STATICSTRING_H
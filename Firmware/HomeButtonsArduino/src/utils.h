#ifndef HOMEBUTTON_UTILS_H
#define HOMEBUTTON_UTILS_H

#include <IPAddress.h>
#include "static_string.h"

template <size_t N>
inline StaticString<N> ensure_trailing_slash(const StaticString<N>& url) {
  if (url.length() == 0) {
    return url;
  }
  if (url.length() > 0 && url[url.length() - 1] == '/') {
    return url;
  }
  return url + "/";
}

inline StaticString<15> ip_address_to_static_string(
    const IPAddress& ip_address) {
  return StaticString<15>("%u.%u.%u.%u", ip_address[0], ip_address[1],
                          ip_address[2], ip_address[3]);
}

#endif  // HOMEBUTTON_UTILS_H
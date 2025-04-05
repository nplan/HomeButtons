#ifndef HOME_BUTTONS_DOWNLOAD_H
#define HOME_BUTTONS_DOWNLOAD_H

#include <FS.h>

namespace download {
bool download_file(const char* url, File& file,
                   const char* certificate = nullptr);

bool check_connection(const char* url, const char* certificate = nullptr);
}  // namespace download
#endif

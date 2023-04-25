#ifndef HOME_BUTTONS_DOWNLOAD_H
#define HOME_BUTTONS_DOWNLOAD_H

#include <FS.h>

namespace download {
bool download_file_https(const char* host, const char* url, File& file,
                         const char* certificate);

bool check_connection(const char* host, const char* url,
                      const char* certificate);
}  // namespace download
#endif

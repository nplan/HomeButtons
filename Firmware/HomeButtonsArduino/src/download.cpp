#include "download.h"

#include <stdint.h>
#include <WiFiClientSecure.h>

#include <HTTPClient.h>

#include "config.h"
#include "logger.h"
#include "static_string.h"

static constexpr uint32_t DOWNLOAD_TIMEOUT = 5000;
static constexpr size_t DOWNLOAD_BUFFER_SIZE = 1024;

bool download::download_file_https(const char* host, const char* url,
                                   File& file, const char* certificate) {
  static Logger logger("Download");

  // Send a GET request for the BMP file
  HTTPClient https;
  https.setConnectTimeout(DOWNLOAD_TIMEOUT);
  https.setTimeout(DOWNLOAD_TIMEOUT);
  https.begin(url, certificate);
  int http_code = https.GET();
  if (http_code != HTTP_CODE_OK) {
    logger.error("GET request failed with code %d", http_code);
    https.end();
    return false;
  }

  // Write the BMP data to the file
  WiFiClient* stream = https.getStreamPtr();
  int totalBytes = 0;
  uint32_t start_time = millis();
  while (https.connected() && (totalBytes < https.getSize())) {
    if (stream->available()) {
      uint8_t buffer[DOWNLOAD_BUFFER_SIZE];
      int bytesRead = stream->readBytes(buffer, sizeof(buffer));
      if (bytesRead == 0) {
        break;
      }
      file.write(buffer, bytesRead);
      totalBytes += bytesRead;
    }
    if (millis() - start_time > DOWNLOAD_TIMEOUT) {
      logger.error("Download timed out");
      https.end();
      file.close();
      return false;
    }
    delay(1);
  }
  file.close();
  logger.debug("Wrote %d bytes", totalBytes);

  https.end();
  logger.debug("Disconnected from server");
  return true;
}

bool download::check_connection(const char* host, const char* url,
                                const char* certificate) {
  static Logger logger("Download");

  // Send a GET request for the BMP file
  HTTPClient https;
  https.setConnectTimeout(DOWNLOAD_TIMEOUT);
  https.setTimeout(DOWNLOAD_TIMEOUT);
  https.begin(url, certificate);
  int http_code = https.GET();
  if (http_code != HTTP_CODE_OK) {
    logger.error("GET request failed with code %d", http_code);
    return false;
  } else {
    return true;
  }
  https.end();
}
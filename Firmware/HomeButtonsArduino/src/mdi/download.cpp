#include "download.h"

#include <stdint.h>
#include <WiFiClientSecure.h>

#include <HTTPClient.h>

#include "config.h"
#include "logger.h"
#include "static_string.h"

static constexpr uint32_t DOWNLOAD_TIMEOUT = 10000;
static constexpr size_t DOWNLOAD_BUFFER_SIZE = 1024;

bool download::download_file(const char* url, File& file,
                             const char* certificate) {
  static Logger logger("Download");

  // Send a GET request for the BMP file
  HTTPClient https;
  https.setConnectTimeout(DOWNLOAD_TIMEOUT);
  https.setTimeout(DOWNLOAD_TIMEOUT);
  if (certificate != nullptr) {
    logger.debug("Using certificate");
    https.begin(url, certificate);
  } else {
    logger.debug("Not using certificate");
    https.begin(url);
  }
  int http_code = https.GET();
  if (http_code != HTTP_CODE_OK) {
    logger.error("GET request failed with code %d", http_code);
    https.end();
    return false;
  }

  // Write the BMP data to the file
  WiFiClient* stream = https.getStreamPtr();
  int16_t content_size = https.getSize();
  if (content_size == -1) {
    logger.error("No content length");
    https.end();
    return false;
  }
  size_t totalBytes = 0;
  uint32_t start_time = millis();
  while (totalBytes < content_size) {
    if (stream->available()) {
      size_t num_2_read;
      if (content_size - totalBytes < DOWNLOAD_BUFFER_SIZE) {
        num_2_read = content_size - totalBytes;
      } else {
        num_2_read = DOWNLOAD_BUFFER_SIZE;
      }
      uint8_t buffer[DOWNLOAD_BUFFER_SIZE];
      size_t bytesRead = stream->readBytes(buffer, num_2_read);
      file.write(buffer, bytesRead);
      totalBytes += bytesRead;
    }
    if (millis() - start_time > DOWNLOAD_TIMEOUT) {
      logger.error("Download timed out");
      https.end();
      file.close();
      return false;
    }
    yield();
  }
  file.close();
  logger.debug("Wrote %d bytes", totalBytes);

  https.end();
  logger.debug("Disconnected from server");
  return true;
}

bool download::check_connection(const char* url, const char* certificate) {
  static Logger logger("Download");

  // Send a GET request for the BMP file
  HTTPClient https;
  https.setConnectTimeout(DOWNLOAD_TIMEOUT);
  https.setTimeout(DOWNLOAD_TIMEOUT);
  if (certificate != nullptr) {
    logger.debug("Using certificate");
    https.begin(url, certificate);
  } else {
    logger.debug("Not using certificate");
    https.begin(url);
  }
  int http_code = https.GET();
  https.end();
  logger.info("GET request to %s returned %d", url, http_code);
  if (http_code < 0) {
    return false;
  } else {
    return true;
  }
}
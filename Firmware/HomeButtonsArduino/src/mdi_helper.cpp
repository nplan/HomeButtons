#include "mdi_helper.h"

#include "download.h"
#include "github_raw_cert.h"

static constexpr char HOST[] = "raw.githubusercontent.com";

static constexpr char MDI_URL[] =
    "https://raw.githubusercontent.com/nplan/MDI-BMP/main/64x64/";

static constexpr char TEST_URL[] =
    "https://raw.githubusercontent.com/nplan/MDI-BMP/main/64x64/alien.bmp";

static constexpr char FOLDER[] = "/mdi";

bool MDIHelper::begin() {
  if (spiffs_mounted_) {
    return true;
  }
  if (!SPIFFS.begin()) {
    error("Failed to mount SPIFFS file system");
    return false;
  }

  spiffs_mounted_ = true;
  debug("Mounted SPIFFS file system");
  return true;
}

void MDIHelper::end() {
  if (!spiffs_mounted_) {
    return;
  }
  SPIFFS.end();
  spiffs_mounted_ = false;
  debug("Unmounted SPIFFS file system");
}

void MDIHelper::get_path(char* out, const char* name) {
  StaticString<64> path("%s/%s.bmp", FOLDER, name);
  strcpy(out, path.c_str());
}

bool MDIHelper::check_connection() {
  return download::check_connection(
      HOST, TEST_URL, github_raw_cert::cert_DigiCert_TLS_RSA_SHA256_2020_CA1);
}

bool MDIHelper::download(const char* name) {
  if (!spiffs_mounted_) {
    error("SPIFFS not mounted");
    return false;
  }

  char path[64];
  get_path(path, name);

  if (SPIFFS.exists(path)) {
    info("'%s' already exists", name);
    return true;
  }

  debug("Downloading '%s' to '%s'", name, path);

  File file = SPIFFS.open(path, FILE_WRITE, true);
  if (!file) {
    error("Failed to open '%s' for writing", path);
    return false;
  }

  StaticString<256> url("%s%s.bmp", MDI_URL, name);
  bool ret = download::download_file_https(
      HOST, url.c_str(), file,
      github_raw_cert::cert_DigiCert_TLS_RSA_SHA256_2020_CA1);
  if (ret) {
    info("Downloaded '%s'", name);
    return true;
  } else {
    error("Failed to download '%s'", name);
    SPIFFS.remove(path);
    return false;
  }
}

bool MDIHelper::exists(const char* name) {
  if (!spiffs_mounted_) {
    error("SPIFFS not mounted");
    return false;
  }
  char path[64];
  get_path(path, name);
  return SPIFFS.exists(path);
}

File MDIHelper::get_file(const char* name) {
  if (!spiffs_mounted_) {
    error("SPIFFS not mounted");
    return File();
  }

  if (!exists(name)) {
    error("'%s' does not exist", name);
    return File();
  }

  char path[64];
  get_path(path, name);
  debug("Opening '%s'", path);
  return SPIFFS.open(path, FILE_READ);
}

size_t MDIHelper::get_free_space() {
  if (!spiffs_mounted_) {
    error("SPIFFS not mounted");
    return 0;
  }
  size_t free = SPIFFS.totalBytes() - SPIFFS.usedBytes();
  debug("Free space: %d", free);
  return free;
}

bool MDIHelper::make_space(size_t size) {
  if (!spiffs_mounted_) {
    error("SPIFFS not mounted");
    return false;
  }
  if (get_free_space() > size) {
    return true;
  }
  info("Freeing space...");
  File root = SPIFFS.open(FOLDER);
  if (!root) {
    error("Failed to open '%s'", FOLDER);
    return false;
  }
  if (!root.isDirectory()) {
    error("'%s' is not a directory", FOLDER);
    return false;
  }
  uint16_t count = 0;
  size_t size_before = SPIFFS.usedBytes();
  while (get_free_space() < size) {
    File file = root.openNextFile();
    if (!file) {
      error("Failed to open next file");
      continue;
    }
    char path[64];
    strcpy(path, file.path());
    file.close();
    SPIFFS.remove(path);
    count++;
    debug("Removed '%s'", path);
    delay(10);
  }
  info("Removed %d files, freed %d bytes", count,
       size_before - SPIFFS.usedBytes());
  return true;
}

bool MDIHelper::remove(const char* name) {
  if (!spiffs_mounted_) {
    error("SPIFFS not mounted");
    return false;
  }
  char path[64];
  get_path(path, name);
  debug("Removing '%s'", path);
  return SPIFFS.remove(path);
}

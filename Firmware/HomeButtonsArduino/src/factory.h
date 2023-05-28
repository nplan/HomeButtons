#ifndef HOMEBUTTONS_FACTORY_H
#define HOMEBUTTONS_FACTORY_H

#include <IPAddress.h>

#include "static_string.h"
#include "logger.h"

class Display;
struct HardwareDefinition;

struct FacTestParams {
  bool do_test;
  String wifi_ssid;
  String wifi_password;
  IPAddress mqtt_server;
  uint32_t mqtt_port;
  String mqtt_user;
  String mqtt_password;
};

struct TestSpec {
  bool received;
  float temp_ref;
  float temp_tol;
  float humd_ref;
  float humd_tol;
  int16_t batt_mvolt_ref;
  int16_t batt_mvolt_tol;
  StaticString<56> mdi_name;
  StaticString<32> disp_text;
};

class FactoryTest : public Logger {
 public:
  FactoryTest() : Logger("Fac.Test") {}
  bool is_test_required();
  bool run_test(HardwareDefinition& HW, Display& display);

 private:
  TestSpec test_spec_ = {};

  void _mqtt_callback(const char* topic, uint8_t* payload, uint32_t length);
};

#endif  // HOMEBUTTONS_FACTORY_H

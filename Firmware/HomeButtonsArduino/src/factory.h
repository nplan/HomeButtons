#ifndef HOMEBUTTONS_FACTORY_H
#define HOMEBUTTONS_FACTORY_H

#include <PubSubClient.h>
#include <WiFi.h>

#include "display.h"
#include "hardware.h"

namespace factory
{
  void factory_mode(DeviceState& device_state, Display& display);
}

#endif // HOMEBUTTONS_FACTORY_H

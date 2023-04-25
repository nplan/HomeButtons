#ifndef HOMEBUTTONS_FACTORY_H
#define HOMEBUTTONS_FACTORY_H

class DeviceState;
class Display;
struct HardwareDefinition;

namespace factory {
void factory_mode(HardwareDefinition& HW, Display& display);
}

#endif  // HOMEBUTTONS_FACTORY_H

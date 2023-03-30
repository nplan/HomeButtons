#ifndef HOMEBUTTONS_SETUP_H
#define HOMEBUTTONS_SETUP_H

class DeviceState;
class Display;
struct HardwareDefinition;

void start_wifi_setup(DeviceState& device_state, Display& display);
void start_setup(DeviceState& device_state, Display& display,
                 HardwareDefinition& HW);

#endif  // HOMEBUTTONS_SETUP_H

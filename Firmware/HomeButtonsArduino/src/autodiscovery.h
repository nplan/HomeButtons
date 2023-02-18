#ifndef HOMEBUTTONS_AUTODISCOVERY_H
#define HOMEBUTTONS_AUTODISCOVERY_H

class DeviceState;
class Network;

void send_discovery_config(const DeviceState& device_state, Network& network);
void update_discovery_config(const DeviceState& device_state, Network& network);

#endif // HOMEBUTTONS_AUTODISCOVERY_H

# Help

When common issues are detected, solutions will be posted here.

If you encounter an issue contact us at [support@home-buttons.com](mailto:support@home-buttons.com)
 or create a new issue in the Github repository.

### 1. Wi-Fi setup Hotspot not appearing
It's an issue with stock firmware (v2.0.3) on a small number of devices
Please flash the firmware again via USB following this [guide](update.md#USB). Use the latest release.
(Flashing v2.0.3 over v2.0.3 again will solve the issue.)

### 2. Text fields for labels not appearing in Home Assistant
It's only supported in Home Assistant 2022.12 and later. Please update Home Assistant.

### 3. Long Wi-Fi reaction time
The reaction time depends heavily on your Wi-Fi network. In congested networks or if the signal is weak where *Home Buttons* is installed, reaction time will be longer.

Reaction time is also longer on WPA3 due to additional encryption compared to WPA2.

You can considerably decrease the reaction time by setting up static IP for your *Home Buttons*.

The expected average values for reaction time are:

- WPA2 static IP: **0.5 s**
- WPA2 DHCP: **1 s**
- WPA3 static IP: **1.8 s**
- WPA3 DHCP: **2.7 s**

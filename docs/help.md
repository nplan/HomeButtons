# Frequent Issues

> If you encounter an issue contact us at [support@home-buttons.com](mailto:support@home-buttons.com)
 or create a new issue in the *Github* [repository](https://github.com/nplan/HomeButtons).

### 1. Wi-Fi setup Hotspot not appearing
It's an issue with stock firmware (v2.0.3) on a small number of devices
Please flash the firmware again via USB following this [guide](update.md#USB). Use the latest release.
(Flashing v2.0.3 over v2.0.3 again will solve the issue.)

### 2. Text fields for labels not appearing in Home Assistant
It's only supported in *Home Assistant* 2022.12 and later. Please update *Home Assistant*.

### 3. Long Wi-Fi reaction time
The reaction time depends heavily on your Wi-Fi network. In congested networks or if the signal is weak where *Home Buttons* is installed, reaction time will be longer.

Reaction time is also longer on WPA3 due to additional encryption compared to WPA2.

You can considerably decrease the reaction time by setting up static IP for your *Home Buttons*.

The expected average values for reaction time are:

- WPA2 static IP: **0.5 s**
- WPA2 DHCP: **1.0 s**
- WPA3 static IP: **2.0 s**
- WPA3 DHCP: **3.0 s**

### 4. Device not appearing in Home Assistant

You might need to trigger a publish of the MQTT discovery message. To do that, enter [Settings Menu](user_guide.md#settings) by pressing any button for 5 s, and restart the device by pressing :material-restore:. The discovery message will be published on the next button press after the restart.

### 5. Display showing :material-file-question-outline: instead of an icon

Please check that the icon name is correct and that you put `mdi:` prefix before the name.

Another reason could be that the download of the icons failed.

To trigger download again manually, enter [Settings Menu](user_guide.md#settings) by pressing any button for 5 s, and restart the device by pressing :material-restore:. The missing icons will be downloaded on the next button press after the restart.
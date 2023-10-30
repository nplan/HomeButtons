# Frequent Issues

> If you encounter an issue contact us at [support@home-buttons.com](mailto:support@home-buttons.com)
 or create a new issue in the *Github* [repository](https://github.com/nplan/HomeButtons).

### 1. Text fields for labels not appearing in Home Assistant
It's only supported in *Home Assistant* 2022.12 and later. Please update *Home Assistant*.

### 2. Long Wi-Fi reaction time
The reaction time depends heavily on your Wi-Fi network. In congested networks or if the signal is weak where *Home Buttons* is installed, reaction time will be longer.

Reaction time is also longer on WPA3 due to additional encryption compared to WPA2.

You can considerably decrease the reaction time by setting up static IP for your *Home Buttons*.

The expected average values for reaction time are:

- WPA2 static IP: **0.5 s**
- WPA2 DHCP: **1.0 s**
- WPA3 static IP: **2.0 s**
- WPA3 DHCP: **3.0 s**

### 3. Device not appearing in Home Assistant

You might need to trigger a publish of the MQTT discovery message. To do that, enter [Settings Menu](original/user_guide.md#settings) by pressing any button for 5 s, and restart the device by pressing :material-restore:. The discovery message will be published on the next button press after the restart.

### 4. Display showing :material-file-question-outline: instead of an icon

Please check that the icon name is correct and that you put `mdi:` prefix before the name.

Another reason could be that the download of the icons failed.

To trigger download again manually, enter [Settings Menu](original/user_guide.md#settings) by pressing any button for 5 s, and restart the device by pressing :material-restore:. The missing icons will be downloaded on the next button press after the restart.

### 5. Bricked device

This can happen if you update to v2.4.0 or later directly from a version older than v2.1.0. Follow this instruction to recover your device:

1. Connect device with an USB-C cable to your computer
1. Go to [Home Buttons Flasher](https://nplan.github.io/HomeButtonsFlasher/)
1. Put device to programming mode by holding BOOT button and pressing RST button. Boot LED should light up.
1. Select **Home Buttons**, **V2.4.0** and **Full Image**
1. Click Connect, select ESP32-S2 from the list and click Connect
1. Click Install V2.4.0 ORIGINAL FULL IMAGE
1. Check Erase device and click Next
1. Confirm installation
1. Wait about 2 minutes for installation to complete
1. Close the browser tab and go to [Adafruit ESPTool](https://adafruit.github.io/Adafruit_WebSerial_ESPTool/)
1. On device hold BOOT button and press RST button. Boot LED should light up.
1. Select 921600 Baud and click Connect
1. Select ESP32-S2 from the list & click Connect
1. In the first offset field enter 9000 and choose the provided **NVS bin file**
1. Click Program and wait a few seconds
1. On device press RST button. Home Buttons will show welcome screen.
1. Set up the device again according to the getting started guide

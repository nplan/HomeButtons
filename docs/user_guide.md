# User guide


## Controlling The Device

Press any button for:

- 2 s - [Info Screen](#info_screen)
- 10 s - [Setup](#setup)
- 20 s - [Wi-Fi Setup](#wifi_setup)
- 30 s - [Factory Reset](#factory_reset)

## Home Assistant

You can configure most of the devices settings directly in *Home Assistant*.

> To get to the device's page in *Home Assistant*, click settings in the left side bar, then open *Devices & Services*, move to the *Devices* tab and click on the name you gave your *Home Buttons* during setup.

![Home Assistant Device Page](assets/home_assistant_device.png){width="500"}

### Configure Button Labels

In the *Controls* card, enter the button labels that you want to be shown on the e-paper display. The buttons are ordered from top to bottom. Labels will be updated next time you press a button or on the next sensor update interval.

> *Home Buttons* will choose font size automatically. It can display around **5** letters in large font and around **7** letters in smaller font.
Labels over **7** letters will be clipped. Choose what you want to display wisely :)

> Labels support UTF-8 with special characters. If a character is not available in the display font, it will be skipped.

### Configure Button Actions

To configure button actions, click "+" on the *Automations* card, select one of the buttons and set up an automation with *Home Assistant*'s editor.

![Home Assistant Triggers](assets/home_assistant_triggers.png){width="350"}

> The expected delay from a button being pressed to the automation being triggered is around 1 second.

### Configure Sensor Publish Interval {#sensor_interval}

The device uses deep sleep to preserve battery. It wakes up every few minutes to measure temperature and humidity and publish the data to MQTT topics. You can set the publishing interval with a slider on the *Controls* card.
The range is from 1 to 30 minutes. The default is 10 minutes.

> Be aware, that this setting greatly impacts the battery life. The advertised battery life of > 1.5 year is achievable with the interval set to 10 minutes (the default) or greater.

> `Sensor Interval` parameter will only be used in *Sleep Mode*. In *Awake Mode* the sensor publish interval is 60 seconds.

### Awake Mode

*Home Buttons* supports two modes of operation:

1. **Sleep Mode** 

    The default. *Home Buttons* will spend most of the time in deep sleep. It will wake up and connect to network only when a button is pressed or every few minutes as specified by the [`Sensor Interval`](#sensor_interval) option.
    When changing button labels, the display will only be updated at the next wake up.

2. **Awake Mode**

    Available only when USB-C is connected or if device is powered by the 5 V DC input. In this mode the device is always on and connected to network. Changing button labels will update the display instantly.
    The sensor publish interval is fixed at 60 seconds. The disadvantage of this mode is that the temperature sensor reading will be a few degrees too high, since the Wi-Fi modem is generating heat.

> *Awake Mode* is only available on *Home Buttons* hardware revision >= 2.2 by default. If you wish to enable it on earlier revisions, you must perform a [hardware hack](hardware_hacks.md#usb-power-without-battery).


## Info Screen {#info_screen}

Info screen displays current temperature, humidity and battery charge percentage.

!["Info Screen](assets/info_screen.png){width="125"}

Bring it up by pressing any button for 2 seconds. *Home Buttons* will automatically revert to showing button labels in 30 seconds. Or do that manually by pressing any button again.

## Setup {#setup}

Setup allows you to change connection settings and button labels. *Home Buttons* establishes a web interface accessible within the network it is already connected to.

Begin setup by pressing any button for 10 seconds. *Home Buttons* will display instructions for connecting to a web interface. Scan the QR code or enter the local IP into a web browser.

![Setup Screen](assets/setup_screen.png){width="125"} 
![Setup Page](assets/setup_page.jpeg){width="250"}

### Change Wi-Fi settings

Click `Configure WiFi` to change Wi-Fi connection settings. Select a network, enter the password and click save. Wait a few seconds and then press any button to exit setup. *Home Buttons* will connect to the newly selected Wi-Fi network.

### Change MQTT settings & Button labels

Click `Setup` to change MQTT settings or button labels. A page with the following parameters will open:

- `Device Name` - Name of your device as it will appear in *Home Assistant*.

- `MQTT Server` - IP address of your MQTT broker. Usually the same as IP of your *Home Assistant* server.

- `MQTT Port` - Port used by MQTT broker. The default is usually *1883*.

- `MQTT User` - MQTT user name (can be empty if not required by broker).

- `MQTT Password` - MQTT password (can be empty if not required by broker).

- `Base Topic` - MQTT topic that will be prepended to all topics used by *Home Buttons*. The default is `homebuttons`.

- `Discovery Prefix` - *Home Assistant* parameter for MQTT discovery. The default is `homeassistant`.
Leave that unchanged if you haven't modified *Home Assistant*'s configuration.

- `Static IP` - Optional. IP of *Home Buttons*. Must be outside the DHCP address range of your router.

- `Gateway` - Optional. The IP address of your router.

- `Subnet Mask` Optional. Usually `255.255.255.0.`

- `Button {1-6} Label` - Label that will be displayed next to each button. The order is from top to bottom.

> *Home Buttons* will choose font size automatically. It can display around **5** letters in large font and around **7** letters in smaller font.
Labels over **7** letters will be clipped. Choose what you want to display wisely :)

> ![Label Text Size Comparison](assets/text_sizes.png){width="125"}

When done, click `Save`. Device will exit the setup and display button labels.

> If MQTT connection is not successful, `MQTT error` will be displayed and *Home Buttons* will return to welcome screen.
You can start the setup again by pressing any button. Please make sure to enter correct MQTT parameters.

## Wi-Fi Setup {#wifi_setup}

If *Home Buttons* becomes inaccessible on the local network due to changed Wi-Fi settings, you can restart the Wi-Fi setup at any time.

Press any button for 20 seconds.

1. *Home Buttons* will establish a Wi-Fi hotspot.
Connect to it by scanning the QR code on the display or manually connecting to Wi-Fi network and entering the password.

    ![Wi-Fi Setup Screen](assets/wifi_setup_screen.png){width="125"}

2. After connecting to *Home Buttons* Wi-Fi with your device, a **captive portal** will pop up automatically.
If it doesn't, open the web browser and navigate to any web page. You will be redirected to the captive portal.

    ![Wi-Fi Setup Page](assets/wifi_setup_page_1.png){width="200"}

3. Click `Configure WiFi` and wait a few seconds for a list of networks to appear.

4. Select your network, enter the password and click `Save`.

*Home Buttons* will disable the hotspot and connect to your Wi-Fi network in a few seconds. `Wi-Fi CONNECTED` will appear on display.

> If connection is not successful, `Wi-Fi error` will be displayed and *Home Buttons* will return to welcome screen.
You can start Wi-Fi setup again by pressing any button. Please make sure to enter the password correctly.

## Factory Reset {#factory_reset}

Factory reset deletes all user settings and returns the device to its initial state.

Perform the factory reset by pressing any button for 30 seconds. `Factory RESET...` will appear on the display and *Home Buttons* will restart and display welcome screen.
You can complete the setup again by following the [Getting Started](setup.md) guide. 

## Opening the case {#opening_case}

If you need to remove or replace the battery, or perform a manual firmware upgrade, you have to open the case. The back cover can stay mounted to the wall during the procedure.

1. Detach the back cover by inserting a flat headed screwdriver in the hole at the bottom and twisting it.

    ![Open back cover image](assets/open_back_cover.jpeg){width="300"}

2. Pull off the front of device. You may need to use some force.
Hold device in the corners, where the case is strongest.

3. When you're done, reposition the front of device and push firmly until it's flush with the wall.

    ![Mount To Wall](assets/mount_2_wall_2.jpeg){width="300"}
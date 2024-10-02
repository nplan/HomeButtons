# MQTT Topics

*Home Buttons* uses MQTT to communicate with you smart home.

Here is a list of topics that are used by the device:

Topic | Description | Retained
------| ----------- | --------
{BASE_TOPIC}/{DEVICE_NAME}/available | Online status of the device. "online" or "offline" is published to this topic. | Yes
{BASE_TOPIC}/{DEVICE_NAME}/system_state | A json object with info like uptime, Wi-Fi signal strength, etc. | Yes
{BASE_TOPIC}/{DEVICE_NAME}/button_{1-4} | When button {1-4} is pressed, "PRESS is published to this topic. | No
{BASE_TOPIC}/{DEVICE_NAME}/button_{1-4}_double | When button {1-4} is pressed 2 times, "PRESS is published to this topic. | No
{BASE_TOPIC}/{DEVICE_NAME}/button_{1-4}_triple | When button {1-4} is pressed 3 times, "PRESS is published to this topic. | No
{BASE_TOPIC}/{DEVICE_NAME}/button_{1-4}_quad | When button {1-4} is pressed 4 times, "PRESS is published to this topic. | No
{BASE_TOPIC}/{DEVICE_NAME}/switch_{1-4} | Switch {1-4} state. "ON" or "OFF". | No
{BASE_TOPIC}/{DEVICE_NAME}/cmd/switch_{1-4} | Switch {1-4} command. "ON" or "OFF". | No
{BASE_TOPIC}/{DEVICE_NAME}/led_amb_bright | Ambient LED brightness state. | No
{BASE_TOPIC}/{DEVICE_NAME}/cmd/led_amb_bright | Ambient LED brightness command. | No

- {BASE_TOPIC} - Configured during setup. Default is *homebuttons*.
- {DEVICE_NAME} - Name of device as configured during setup and shown in *Home Assistant*
# MQTT Topics

*Home Buttons* uses MQTT to communicate with you smart home.

Here is a list of topics that are used by the device:

Topic | Description | Retained
------| ----------- | --------
{BASE_TOPIC}/{DEVICE_NAME}/button_{1-6} | When button {1-6} is pressed, "PRESS is published to this topic. | No
{BASE_TOPIC}/{DEVICE_NAME}/temperature | Temperature in °C. Published on button press and every  N minutes, specified by *Sensor Interval*. | No
{BASE_TOPIC}/{DEVICE_NAME}/humidity | Relative humidity in %. Published on button press and  every  N minutes, specified by *Sensor Interval*. | No
{BASE_TOPIC}/{DEVICE_NAME}/battery | Relative humidity in %. Published on button press and  every  N minutes, specified by *Sensor Interval*. | No
{BASE_TOPIC}/{DEVICE_NAME}/btn_{1-6}_label | Current label of button {1-6}.| Yes
{BASE_TOPIC}/{DEVICE_NAME}/sensor_interval | Current sensor publish interval in minutes. | Yes
{BASE_TOPIC}/{DEVICE_NAME}/cmd/btn_{1-6}_label | Command to change label of button {1-6} to new value. Topic cleared by device when received. | Yes
{BASE_TOPIC}/{DEVICE_NAME}/cmd/sensor_interval | Command to change sensor publish interval. 1 - 30 minutes. Topic cleared by device when received. | Yes

- {BASE_TOPIC} - Configured during setup. Default is *homebuttons*.
- {DEVICE_NAME} - Name of device as configured during setup and shown in *Home Assistant*
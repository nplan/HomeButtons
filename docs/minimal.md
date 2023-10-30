# Minimal Setup
This guide explains how to get your *Home Buttons* device working with the
minimal amount of setup. The only requirement is an MQTT broker.
*Home Assistant* is not required.

The required steps are:

1. Connect your *Home Buttons* to your Wi-Fi network
2. Set up an MQTT broker
3. Connect *Home Buttons* to the MQTT broker
4. Test if button presses are working

## Wi-Fi Setup
See [Set Up Wi-Fi Connection](original/setup.md#setup_wifi){:target="_blank"} section of the Getting Started guide for how to get your
*Home Buttons* device connected to your Wi-Fi network.

## MQTT broker
You will need a working MQTT broker to use *Home Buttons*. ***mosquitto*** is a good choice. To get it running
as quickly as possible, run the following on a Debian server / VM:

```sh
# Install mosquitto and some other tools
sudo apt install -y mosquitto mosquitto-clients jq

# Tell mosquitto to listen on an interface other than loopback
echo -n "bind_address "
ip -j a | jq -r '.[] | select(.ifname != "lo").addr_info[] | select(.family == "inet").local' | sudo tee -a /etc/mosquitto/conf.d/network.conf
# The above command just figures out your IP address and puts "bind_address IP"
# in /etc/mosquitto/conf.d/network.conf

# Restart mosquitto so it is listening on the network interface (not the loopback)
systemctl restart mosquitto
```

At this point running `sudo ss -ltpn | grep mosquitto` should show that
*mosquitto* is listening on port 1883. If that command does not return anything,
something has gone wrong. In this case, check the
[mosquitto documentation](https://mosquitto.org/documentation/){:target="_blank"} to identify and
resolve the issue.

## Connect *Home Buttons* to your MQTT broker
See the [Set Up MQTT connection](original/setup.md#setup_mqtt){:target="_blank"} section of the Getting Started guide
for how to get your *Home Buttons* connected to your MQTT broker.

## Testing
*Home Buttons* communicates by sending messages to various MQTT topics.
For example, when you press a button, *Home Buttons* will send
a message to your MQTT broker. It's up to other devices to listen for these
messages and perform actions.

The list of [MQTT topics](original/mqtt_topics.md){:target="_blank"} is vital to understand in order to
test or troubleshoot your system. Below is an example of how you could view the
messages that are sent when the first button (top left) is pressed. The command
will not print anything until it receives a message.

```sh
mosquitto_sub -h 192.168.0.109 -p 1883 -t "homebuttons/Home Buttons Kitchen/button_1"
```

The example above assumes:

- that the IP address of the *Home Buttons* device is `192.168.0.109`
- the device name is `Home Buttons Kitchen` (configured during setup)
- the base topic is `homebuttons` (the default)

If you press the first button, you should see `PRESS` appear on your console.

Similarly, using `mosquitto_sub` with `button_2_double` instead of `button_1`
will result in `PRESS` appearing when the second button (top right) is
pressed twice.

If this is working, you have confirmed that:

- Your MQTT broker is working
- Your *Home Buttons* device is on the network and connected to your MQTT broker
- The buttons are working as expected

## Additional tests
When in sleep mode (which is the default), *Home Buttons* will only
wake up every few minutes to check in with the MQTT broker.

To see how often the updates happen, check the sensor_interval:

```sh
mosquitto_sub -h 192.168.0.109 -p 1883 -t "homebuttons/Home Buttons Kitchen/sensor_interval"
```

To change how often this happens, use `mosquitto_pub` with the `-r` flag (retain)
and change the path to include `/cmd` like so:

```sh
mosquitto_pub -h 192.168.0.109 -p 1883 -t "homebuttons/Home Buttons Kitchen/cmd/sensor_interval" -m "1" -r
```

The retain flag instructs the MQTT broker to hold onto the message, so it can be
sent to non connected (sleeping) clients later, when they connect. 

A similar command can be used to update the button labels:

```sh
mosquitto_pub -h 192.168.0.109 -p 1883 -t "homebuttons/Home Buttons Kitchen/cmd/btn_1_label" -m "Look" -r
mosquitto_pub -h 192.168.0.109 -p 1883 -t "homebuttons/Home Buttons Kitchen/cmd/btn_2_label" -m "mom" -r
mosquitto_pub -h 192.168.0.109 -p 1883 -t "homebuttons/Home Buttons Kitchen/cmd/btn_3_label" -m "" -r
mosquitto_pub -h 192.168.0.109 -p 1883 -t "homebuttons/Home Buttons Kitchen/cmd/btn_4_label" -m "A" -r
mosquitto_pub -h 192.168.0.109 -p 1883 -t "homebuttons/Home Buttons Kitchen/cmd/btn_5_label" -m "Message" -r
mosquitto_pub -h 192.168.0.109 -p 1883 -t "homebuttons/Home Buttons Kitchen/cmd/btn_6_label" -m "4 you" -r
```

## Additional setup

Please read the [mosquitto documentation](https://mosquitto.org/documentation/){:target="_blank"}
for more info about the configuration options.
It's likely that you will at least want to add authentication to your MQTT broker.

# Overview
This document explains how to get your HomeButtons device working with the
minimal amount of setup. These instructions do not require setting up a Home
Assistant server. The general process is as follows:

1. Get your HomeButtons on your wifi network
2. Set up an MQTT server
3. Connect HomeButtons to your MQTT server
4. Test homebuttons

# Device setup
See [Set Up Wi-Fi Connection](setup.md) instructions from the Getting started
guide for info on how to get your HomeButtons device onto your wifi network.

# Mosquitto server
You will need a MQTT server running to use HomeButtons. To get this running
as quickly and easily as possible, run the following on a Debian server/VM:

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
mosquitto is listening on port 1883. If that command does not reutrn anything,
something has gone wrong. In this case, you will likely want to read the
[mosquitto documentation](https://mosquitto.org/documentation/) to identify and
resolve the issue.

# Connect HomeButtons to your MQTT server
See the [Set Up MQTT connection](setup.md) section of the Getting started guide
for how to get your HomeButtons to connect to your MQTT server.

# Testing
There are various times when HomeButtons will send a message to various topics
on the MQTT server. For example, when you press a button, HomeButtons will send
a message to your MQTT server. It's up to other things to listen for these
messages and perform some action.

The list of [MQTT topics](mqtt_topics.md) are vital to understant in order to
test or troubleshoot your system. Below is an example of how you could view the
messages that happen when the first (top left) button is pressed. The command
will not print anything until it receives a message.

```sh
mosquitto_sub -h 192.168.0.109 -p 1883 -t "homebuttons/Home buttons/button_1"
```

The example above assumes:
- that the IP address of the HomeButtons device is 192.168.0.109
- the device name is "Home buttons" (configured during setup)
- the base topic is "homebuttons" (the default)

If you press the first button, you should see "PRESS" appear on your console.

Similarly, using mosquitto_sub with "button_2_double" instead of "button_1"
will result in a PRESS appearing when the second (upper right) button is
pressed twice.

If this is working, you have confirmed a few things:
- Your MQTT broker is working
- Your HomeButtons device is on the network and connected to your MQTT server
- The buttons are working as expected

## Additional tests
When in sleep mode (which is the default), the HomeButtond device will only
wake up every few minutes to check in with the MQTT server.

To see how often the updates will happen, check the sensor_interval:

```sh
mosquitto_sub -h 192.168.0.109 -p 1883 -t "homebuttons/Home buttons/sensor_interval"
```

To change how often this happens, use mosquitto_pub with the -r flag (retain)
and change the path to include `/cmd` like so:

```sh
mosquitto_pub -h 192.168.0.109 -p 1883 -t "homebuttons/Home buttons/cmd/sensor_interval" -m "1" -r
```

The retain flag instructs the MQTT broker to hold onto the message so it can be
sent to the other subscribers (in our case the HomeButtons device). This same
pattern can also be used to update the buttons. For example:

```sh
mosquitto_pub -h 192.168.0.109 -p 1883 -t "homebuttons/Home buttons/cmd/btn_1_label" -m "Look" -r
mosquitto_pub -h 192.168.0.109 -p 1883 -t "homebuttons/Home buttons/cmd/btn_2_label" -m "mom" -r
mosquitto_pub -h 192.168.0.109 -p 1883 -t "homebuttons/Home buttons/cmd/btn_3_label" -m "" -r
mosquitto_pub -h 192.168.0.109 -p 1883 -t "homebuttons/Home buttons/cmd/btn_4_label" -m "A" -r
mosquitto_pub -h 192.168.0.109 -p 1883 -t "homebuttons/Home buttons/cmd/btn_5_label" -m "Message" -r
mosquitto_pub -h 192.168.0.109 -p 1883 -t "homebuttons/Home buttons/cmd/btn_6_label" -m "4 you" -r
```

# Additional setup
It's likely that you will want to add authentication to your MQTT server.

# Home Buttons

This branch contains experimental ESPHome config.

# ATTENTION : at this point in time, there seems to be no path back to original Home Buttons firmware !!!

## Prerequisites

- add your passwords to ESPHome secrets
```
# Your Wi-Fi SSID and password
wifi_ssid: "SSID"
wifi_password: "PASSWORD"
#
mqtt_broker: IP_ADDRESS
mqtt_username: USERNAME
mqtt_password: PASSWORD
```

- follow the [ESPHome font docs](https://esphome.io/components/display/index.html#drawing-static-text)
  The config uses `Roboto-Bold.ttf` font found [at Google Fonts](https://fonts.google.com/specimen/Roboto)

- create a Home Assistant Helper of the `boolean` type, named `Allow deep sleep MQTT`

- create automations :
```
automation:
  - id: some_spiffy_name
    alias: Enable ESPHOME OTA mode
    trigger:
      - platform: state
        entity_id: input_boolean.allow_deep_sleep_mqtt
        to: 'off'
    action:
      - service: script.turn_on
        entity_id: script.ota_on
  #
  - id: another_spiffy_name
    alias: Disable ESPHOME OTA mode
    trigger:
      - platform: state
        entity_id: input_boolean.allow_deep_sleep_mqtt
        to: 'on'
    action:
      - service: script.turn_on
        entity_id: script.ota_off
```

- create scripts : 
```
script:
  ota_on:
    alias: Enable ESPHOME OTA Mode
    icon: mdi:sleep-off
    mode: single
    sequence:
      - data:
          payload: 'ON'
          retain: true
          topic: homeassistant/ota_mode
        service: mqtt.publish
  ota_off:
    alias: Disable ESPHOME OTA Mode
    icon: mdi:sleep
    mode: single
    sequence:
      - service: mqtt.publish
        data:
          topic: homeassistant/ota_mode
          payload: 'OFF'
          retain: true
```

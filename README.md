# Home Buttons

This branch contains experimental ESPHome config.

# Warning

Always backup the stock firmware before flashing with ESPHome:

`python -m esptool -p PORT read_flash 0 0x400000 backup.bin`

Restore stock firmware from backup with:

`python -m esptool -p PORT write_flash 0x0 backup.bin`

This will also backup your Wi-Fi / MQTT settings and button labels.

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

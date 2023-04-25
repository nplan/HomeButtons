#!/usr/bin/env python

from asyncio import open_connection
from dataclasses import dataclass
from time import sleep
from serial import Serial
from datetime import date
from uuid import uuid4
import argparse
import paho.mqtt.client as mqtt

test_topic = "homebuttons-factory/test"
test_payload = "OK"


@dataclass
class FactorySettings:
    serial_number: str = ""
    random_id: str = ""
    model_id: str = ""
    hw_version: str = ""

    def __repr__(self) -> str:
        return f"Factory settings:\n"\
            f"    serial_number: {self.serial_number}\n"\
            f"    random_id: {self.random_id}\n"\
            f"    model_id: {self.model_id}\n"\
            f"    hw_version: {self.hw_version}\n"


@dataclass
class NetworkSettings:
    wifi_ssid: str = ""
    wifi_password: str = ""
    mqtt_server: str = ""
    mqtt_user: str = ""
    mqtt_password: str = ""
    mqtt_port: int = 0


@dataclass
class SensorTargets:
    temperature: float = 0.0
    humidity: float = 0.0


class FactoryHandler:

    def __init__(self) -> None:
        self.serial: Serial = None

    def run_hw_test(self) -> bool:
        print("Starting HW test...", end="", flush=True)
        self.serial.write(b"ST\r")
        while True:
            ret = self.serial.readline().decode().strip()
            if ret == "OK":
                print("OK")
                return True
            else:
                print("FAIL")
                return False

    def set_serial_number(self, serial_number: str) -> bool:
        assert type(serial_number) is str
        assert len(serial_number) == 8
        print("Setting serial number...", end="", flush=True)
        self.serial.write(b"SN-" + serial_number.encode("ascii") + b"\r")
        ret = self.serial.readline().decode().strip()
        if ret == "OK":
            print("OK")
            return True
        else:
            print("FAIL")
            return False

    def set_random_id(self, random_id: str) -> bool:
        assert type(random_id) is str
        assert len(random_id) == 6
        print("Setting random id...", end="", flush=True)
        self.serial.write(b"RI-" + random_id.encode("ascii") + b"\r")
        ret = self.serial.readline().decode().strip()
        if ret == "OK":
            print("OK")
            return True
        else:
            print("FAIL")
            return False

    def set_model_id(self, model_id: str) -> bool:
        assert type(model_id) is str
        assert len(model_id) == 2
        print("Setting model id...", end="", flush=True)
        self.serial.write(b"MI-" + model_id.encode("ascii") + b"\r")
        ret = self.serial.readline().decode().strip()
        if ret == "OK":
            print("OK")
            return True
        else:
            print("FAIL")
            return False

    def set_hw_version(self, hw_version: str) -> bool:
        assert type(hw_version) is str
        assert len(hw_version) == 3
        print("Setting hw version...", end="", flush=True)
        self.serial.write(b"HV-" + hw_version.encode("ascii") + b"\r")
        ret = self.serial.readline().decode().strip()
        if ret == "OK":
            print("OK")
            return True
        else:
            print("FAIL")
            return False

    def save_settings(self) -> bool:
        print("Saving settings...", end="", flush=True)
        self.serial.write(b"OK\r")
        ret = self.serial.readline().decode().strip()
        if ret == "OK":
            print("OK")
            return True
        else:
            print("FAIL")
            return False

    def set_wifi_ssid(self, wifi_ssid: str) -> bool:
        print("Setting wifi ssid...", end="", flush=True)
        self.serial.write(b"WS-" + wifi_ssid.encode("ascii") + b"\r")
        ret = self.serial.readline().decode().strip()
        if ret == "OK":
            print("OK")
            return True
        else:
            print("FAIL")
            return False

    def set_wifi_password(self, wifi_password: str) -> bool:
        print("Setting wifi password...", end="", flush=True)
        self.serial.write(b"WP-" + wifi_password.encode("ascii") + b"\r")
        ret = self.serial.readline().decode().strip()
        if ret == "OK":
            print("OK")
            return True
        else:
            print("FAIL")
            return False

    def set_mqtt_server(self, mqtt_server: str) -> bool:
        print("Setting mqtt server...", end="", flush=True)
        self.serial.write(b"MS-" + mqtt_server.encode("ascii") + b"\r")
        ret = self.serial.readline().decode().strip()
        if ret == "OK":
            print("OK")
            return True
        else:
            print("FAIL")
            return False

    def set_mqtt_user(self, mqtt_user: str) -> bool:
        print("Setting mqtt user...", end="", flush=True)
        self.serial.write(b"MU-" + mqtt_user.encode("ascii") + b"\r")
        ret = self.serial.readline().decode().strip()
        if ret == "OK":
            print("OK")
            return True
        else:
            print("FAIL")
            return False

    def set_mqtt_password(self, mqtt_password: str) -> bool:
        print("Setting mqtt password...", end="", flush=True)
        self.serial.write(b"MP-" + mqtt_password.encode("ascii") + b"\r")
        ret = self.serial.readline().decode().strip()
        if ret == "OK":
            print("OK")
            return True
        else:
            print("FAIL")
            return False

    def set_mqtt_port(self, mqtt_port: int) -> bool:
        print("Setting mqtt port...", end="", flush=True)
        self.serial.write(b"MT-" + str(mqtt_port).encode("ascii") + b"\r")
        ret = self.serial.readline().decode().strip()
        if ret == "OK":
            print("OK")
            return True
        else:
            print("FAIL")
            return False

    def start_wifi_test(self) -> bool:
        print("Starting wifi test...", end="", flush=True)
        self.serial.write(b"TW")
        success = False

        def on_connect(client, userdata, flags, rc):
            client.subscribe(test_topic)

        def on_message(client, userdata, msg):
            nonlocal success
            if msg.payload.decode("ascii") == "OK":
                success = True

        client = mqtt.Client()
        client.on_connect = on_connect
        client.on_message = on_message

        if network_settings.mqtt_user and network_settings.mqtt_password:
            client.username_pw_set(
                network_settings.mqtt_user, network_settings.mqtt_password)
        client.connect(network_settings.mqtt_server,
                       network_settings.mqtt_port, 60)
        client.loop_start()

        while not success:
            sleep(0.1)

        client.loop_stop()
        print("OK")
        return True

    def set_temperature_target(self, target: float) -> bool:
        print("Setting temperature sensor target...", end="", flush=True)
        self.serial.write(b"TT-" + str(target).encode("ascii") + b"\r")
        ret = self.serial.readline().decode().strip()
        if ret == "OK":
            print("OK")
            return True
        else:
            print("FAIL")
            return False

    def set_humidity_target(self, target: float) -> bool:
        print("Setting humidity sensor target...", end="", flush=True)
        self.serial.write(b"TH-" + str(target).encode("ascii") + b"\r")
        ret = self.serial.readline().decode().strip()
        if ret == "OK":
            print("OK")
            return True
        else:
            print("FAIL")
            return False

    def run_factory_setup(self, port, baud, settings: FactorySettings, sensor_targets: SensorTargets,
                          network_settings: NetworkSettings = None):
        with Serial(port, baudrate=baud) as self.serial:
            if not self.set_hw_version(settings.hw_version):
                return
            if not self.set_serial_number(settings.serial_number):
                return
            if not self.set_random_id(settings.random_id):
                return
            if not self.set_model_id(settings.model_id):
                return
            if not self.set_temperature_target(sensor_targets.temperature):
                return
            if not self.set_humidity_target(sensor_targets.humidity):
                return
            if not self.run_hw_test():
                return

            if network_settings:
                if not self.set_wifi_ssid(network_settings.wifi_ssid):
                    return
                if not self.set_wifi_password(network_settings.wifi_password):
                    return
                if not self.set_mqtt_server(network_settings.mqtt_server):
                    return
                if not self.set_mqtt_user(network_settings.mqtt_user):
                    return
                if not self.set_mqtt_password(network_settings.mqtt_password):
                    return
                if not self.set_mqtt_port(network_settings.mqtt_port):
                    return
                if not self.start_wifi_test():
                    return

            print("Settings to be saved: \n", settings)
            if input("Confirm save factory settings? y/n\n") == "y":
                pass
                if self.save_settings():
                    print("DONE. Factory setup successful.")
                else:
                    print("Settings could not be saved.")
            else:
                print("ABORTED. Repeat factory setup.")


if __name__ == "__main__":

    parser = argparse.ArgumentParser(
        description="HomeButtons Factory Setup Tool")
    required = parser.add_argument_group('required arguments')
    optional = parser.add_argument_group('optional arguments')

    required.add_argument("-p", "--port", type=str, required=True)
    required.add_argument("-b", "--baud", type=int, required=True)
    required.add_argument("--serial", type=str, required=True)
    optional.add_argument("--random_id", type=str, required=False)
    required.add_argument("--model_id", type=str, required=True)
    required.add_argument("--hw_version", type=str, required=True)

    required.add_argument("--wifi_ssid", type=str, required=True)
    required.add_argument("--wifi_password", type=str, required=True)
    required.add_argument("--mqtt_server", type=str, required=False)
    required.add_argument("--mqtt_user", type=str, required=False)
    required.add_argument("--mqtt_password", type=str, required=True)
    required.add_argument("--mqtt_port", type=int, required=True)

    required.add_argument("--temperature", type=float, required=True)
    required.add_argument("--humidity", type=float, required=True)

    args = parser.parse_args()

    settings = FactorySettings(
        serial_number=args.serial,
        random_id=args.random_id if args.random_id else str(uuid4())[
            :6].upper(),
        model_id=args.model_id,
        hw_version=args.hw_version
    )

    network_settings = NetworkSettings(
        wifi_ssid=args.wifi_ssid,
        wifi_password=args.wifi_password,
        mqtt_server=args.mqtt_server,
        mqtt_user=args.mqtt_user,
        mqtt_password=args.mqtt_password,
        mqtt_port=args.mqtt_port
    )

    sensor_targets = SensorTargets(
        temperature=args.temperature,
        humidity=args.humidity
    )

    factory = FactoryHandler()
    factory.run_factory_setup(args.port, args.baud,
                              settings, sensor_targets, network_settings)

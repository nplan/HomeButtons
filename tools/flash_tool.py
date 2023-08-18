#!/usr/bin/env python

"""
Flash Tool for Home Buttons

Used to burn eFuses and flash firmware.

Also flashes initial NVS partition with test setup parameters.

If provided, can also flash initial SPIFFS image.

Requires path to directory containing the following files:
- firmware.zip
- inventory.csv (optional)
- model_data.json (optional)
- spiffs.bin (optional)
- test_setup.json

Supports two modes:
- Manual mode: requires user input to put device to boot mode
- Auto mode (--auto): requires a custom test jig based on cp2104

Series mode: (--series)
- If flashing multiple devices in series, the serial number of the first device is required
- The serial number of the next device is automatically incremented
- The serial numbers and random ids are written to 'inventory.csv'

Example usage:
python flash_tool.py firmware_dir --port /dev/ttyUSB0 --baud 921600 \
    --auto --jig_serial 12345678

python flash_tool.py firmware_dir --port /dev/ttyUSB0 --baud 921600 \
    --series --model_id A1 --hw_version 1.1 --serial 2301-001

python flash_tool.py firmware_dir --port /dev/ttyUSB0 --baud 921600 \
    --series --model_id A1 --hw_version 1.1 --serial 2301-001 --random_id A1B2C3

"""

import os
import subprocess
from dataclasses import dataclass
from tempfile import NamedTemporaryFile, TemporaryDirectory
from time import sleep
import csv
import json
import argparse
from uuid import uuid4
from hb_mini_test_jig import HBMiniTestJig
from helpers import *

NVS_GEN_PATH = "components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py"


@dataclass
class FactoryData:
    model_id: str
    hw_version: str
    serial: str
    random_id: str

    @classmethod
    def generate_random_id(cls):
        return str(uuid4())[:6].upper()

    @classmethod
    def verify_model_id(cls, model_id: str):
        # example model_id A1
        try:
            assert len(model_id) == 2
            assert model_id[0].isalpha()
            assert model_id[1].isnumeric()
        except AssertionError:
            return False
        else:
            return True

    @classmethod
    def verify_hw_version(cls, hw_version: str):
        # example hw_version 1.1
        try:
            assert len(hw_version) == 3
            assert hw_version[0].isnumeric()
            assert hw_version[1] == "."
            assert hw_version[2].isnumeric()
        except AssertionError:
            return False
        else:
            return True

    @classmethod
    def verify_serial(cls, serial: str):
        # example serial 2301-001
        serial = serial.split("-")
        try:
            assert len(serial) == 2
            assert len(serial[0]) == 4
            assert len(serial[1]) == 3
        except AssertionError:
            return False
        else:
            return True

    @classmethod
    def verify_random_id(cls, random_id: str):
        # example random_id A1B2C3
        try:
            assert len(random_id) == 6
            assert random_id.isalnum()
        except AssertionError:
            return False
        else:
            return True

    def verify(self):
        return (self.verify_model_id(self.model_id) and
                self.verify_hw_version(self.hw_version) and
                self.verify_serial(self.serial) and
                self.verify_random_id(self.random_id))

    def __repr__(self):
        return ("Factory Data:\n"
                f"  Model ID: {self.model_id}\n"
                f"  HW Version: {self.hw_version}\n"
                f"  Serial: {self.serial}\n"
                f"  Random ID: {self.random_id}")


@dataclass
class FactoryTestSetup:
    wifi_ssid: str
    wifi_password: str
    mqtt_server: str
    mqtt_port: int
    mqtt_username: str
    mqtt_password: str

    @classmethod
    def from_json(cls, json_str: str):
        json_data = json.loads(json_str)
        return cls(**json_data)


@dataclass
class HWModelData:
    model_id: str
    hw_version: str

    @classmethod
    def from_json(cls, json_str: str):
        json_data = json.loads(json_str)
        return cls(**json_data)


def append_inventory(inventory_path: str, factory_data: FactoryData):
    if not os.path.exists(inventory_path):
        with open(inventory_path, "w") as f:
            write = csv.writer(f)
            write.writerow(["model_id", "hw_version", "serial", "random_id"])
    with open(inventory_path, "a") as f:
        write = csv.writer(f)
        write.writerow([factory_data.model_id, factory_data.hw_version,
                       factory_data.serial, factory_data.random_id])


def generate_nvs(out_file_path, size: str, factory_test_params: FactoryTestSetup):
    with NamedTemporaryFile("w") as csv_file:
        writer = csv.writer(csv_file)
        writer.writerow(["key", "type", "encoding", "value"])
        writer.writerow(["fac_test", "namespace", "", ""])
        writer.writerow(["do_test", "data", "u8", "1"])
        writer.writerow(["wifi_ssid", "data", "string",
                        factory_test_params.wifi_ssid])
        writer.writerow(["wifi_pass", "data", "string",
                        factory_test_params.wifi_password])
        writer.writerow(["mqtt_srv", "data", "string",
                        factory_test_params.mqtt_server])
        writer.writerow(["mqtt_port", "data", "u32",
                        factory_test_params.mqtt_port])
        writer.writerow(["mqtt_user", "data", "string",
                        factory_test_params.mqtt_username])
        writer.writerow(["mqtt_pass", "data", "string",
                        factory_test_params.mqtt_password])
        csv_file.flush()

        tokens = ["python", os.path.join(find_espidf(), NVS_GEN_PATH), "generate",
                  csv_file.name, out_file_path, size]
        subprocess.run(tokens, check=True, capture_output=True, text=True)


def decode_factory_data(b: bytearray):
    serial = b[0:8].decode("utf-8")
    random_id = b[8:14].decode("utf-8")
    model_id = b[14:16].decode("utf-8")
    hw_version = b[16:19].decode("utf-8")
    return FactoryData(model_id, hw_version, serial, random_id)


def encode_factory_data(factory_data: FactoryData):
    serial = factory_data.serial
    random_id = factory_data.random_id
    model_id = factory_data.model_id
    hw_version = factory_data.hw_version

    assert len(serial) == 8
    assert len(random_id) == 6
    assert len(model_id) == 2
    assert len(hw_version) == 3

    b = bytearray(32)
    b[0:8] = serial.encode("utf-8")
    b[8:14] = random_id.encode("utf-8")
    b[14:16] = model_id.encode("utf-8")
    b[16:19] = hw_version.encode("utf-8")

    # check
    assert decode_factory_data(b) == factory_data
    return b


def burn_efuses(port: str, baud: int, factory_data: FactoryData):
    factory_data = encode_factory_data(factory_data)
    with NamedTemporaryFile("wb") as f:
        f.write(factory_data)
        f.flush()
        tokens = ["espefuse.py", "--port", port, "--baud", str(baud),
                  "--do-not-confirm",
                  "burn_block_data", "BLOCK3", f.name]
        try:
            subprocess.run(tokens, check=True, capture_output=True, text=True)
        except subprocess.CalledProcessError as e:
            print(e.stdout)
            quit()


def flash_firmware(port: str, baud: int, fw_zip_path: str, test_setup: FactoryTestSetup, spiffs_image_path: str = None):
    with TemporaryDirectory() as tmp_dir:
        # unzip firmware
        unzip_file(fw_zip_path, tmp_dir)
        partitions = load_partition_table(
            os.path.join(tmp_dir, "partitions.csv"))

        # generate NVS image with test setup
        nvs_path = os.path.join(tmp_dir, "nvs.bin")
        generate_nvs(nvs_path, partitions["nvs"]
                     ["Size"], test_setup)

        tokens = ["esptool.py", "--port", port, "--baud", str(baud),
                  "--after", "no_reset", "write_flash",
                  "0x1000", os.path.join(tmp_dir, "bootloader.bin"),
                  "0x8000", os.path.join(tmp_dir, "partitions.bin"),
                  partitions["nvs"]["Offset"], nvs_path,
                  partitions["otadata"]["Offset"], os.path.join(
                      tmp_dir, "ota_data_initial.bin"),
                  partitions["app0"]["Offset"], os.path.join(tmp_dir, "firmware.bin")]

        if spiffs_image_path is not None:
            tokens += [partitions["spiffs"]["Offset"], spiffs_image_path]

        try:
            subprocess.run(tokens, check=True, capture_output=True, text=True)
        except subprocess.CalledProcessError as e:
            print(e.stdout)
            quit()


if __name__ == "__main__":

    parser = argparse.ArgumentParser("Home Buttons Flash Tool")

    parser.add_argument("fw_dir", type=str,
                        help="Directory containing firmware files")

    parser.add_argument("--port", type=str, required=True, help="Serial port")
    parser.add_argument("--baud", type=int, default=921600, help="Serial baud")

    parser.add_argument("--auto", action="store_true",
                        help="Auto mode to use with test jig")
    parser.add_argument("--jig_serial", type=str,
                        help="Serial number of test jig")
    parser.add_argument("--skip_efuse", action="store_true",
                        help="Skip burning eFuses")

    parser.add_argument("--series", action="store_true",
                        help="If flashing multiple devices in series")
    parser.add_argument("--model_id", type=str,
                        help="Model ID of devices")
    parser.add_argument("--hw_version", type=str,
                        help="Hardware version of devices")
    parser.add_argument("--serial", type=str,
                        help="Serial number of device. If 'series', this is the first serial number.")
    parser.add_argument("--random_id", type=str, help="Random ID of device")

    args = parser.parse_args()

    if args.auto and args.jig_serial is None:
        parser.error("--jig_serial is required in auto mode")

    if args.series and args.random_id:
        parser.error("--random_id is not allowed in series mode")

    if not os.path.isdir(args.fw_dir):
        parser.error("Invalid firmware directory")

    factory_data = FactoryData("", "", "", "")

    # load model data
    model_data_path = os.path.join(args.fw_dir, "model_data.json")
    if os.path.exists(model_data_path):
        if args.model_id or args.hw_version:
            parser.error(
                "'--model_id' and '--hw_version' are not allowed when 'model_data.json' exists")
        try:
            with open(model_data_path, "r") as f:
                model_data = HWModelData.from_json(f.read())
        except Exception as e:
            print(e)
            parser.error("Invalid 'model_data.json'")
        factory_data.model_id = model_data.model_id
        factory_data.hw_version = model_data.hw_version
    else:
        if not args.model_id and not args.hw_version:
            parser.error("Missing 'model_data.json'")
        elif not args.model_id or not args.hw_version:
            parser.error("Both '--model_id' and '--hw_version' are required")
        else:
            # data check
            if len(args.model_id) != 2:
                parser.error("Invalid model ID")
            if len(args.hw_version) != 3:
                parser.error("Invalid hardware version")
            factory_data.model_id = args.model_id
            factory_data.hw_version = args.hw_version

    # load firmware
    for file in os.listdir(args.fw_dir):
        if file.endswith(".zip") and file.startswith("firmware"):
            firmware_path = os.path.join(args.fw_dir, file)
            print(f"Found firmware: {file}")
            break
    else:
        parser.error("Missing 'firmware.zip'")

    # load test setup
    test_setup_path = os.path.join(args.fw_dir, "test_setup.json")
    if os.path.exists(test_setup_path):
        try:
            with open(test_setup_path, "r") as f:
                test_setup = FactoryTestSetup.from_json(f.read())
        except Exception as e:
            print(e)
            parser.error("Invalid 'test_setup.json'")
    else:
        parser.error("Missing 'test_setup.json'")

    # load inventory
    if args.series:
        inventory_path = os.path.join(args.fw_dir, "inventory.csv")
        if os.path.exists(inventory_path):
            try:
                with open(inventory_path, "r") as f:
                    read = csv.reader(f)
                    rows = list(read)
                    serial = rows[-1][2]
                    if (not FactoryData.verify_serial(serial)):
                        parser.error("Invalid last serial number")
                    factory_data.serial = increment_serial(serial)
            except Exception as e:
                print(e)
                parser.error("Invalid inventory file")
            if args.serial:
                parser.error(
                    "Inventory file already exists, '--serial' is not allowed")
        else:
            if not args.serial:
                parser.error(
                    "Inventory file not yet existing, '--serial' is required")
            factory_data.serial = args.serial
    else:
        factory_data.serial = args.serial

    # verify factory data
    if args.random_id:
        factory_data.random_id = args.random_id
    else:
        factory_data.random_id = FactoryData.generate_random_id()
    if not factory_data.verify():
        parser.error(f"Invalid factory data:\n {factory_data}")

    # load spiffs image
    spiffs_image_path = os.path.join(args.fw_dir, "spiffs.bin")
    if not os.path.isfile(spiffs_image_path):
        spiffs_image_path = None
        print("No SPIFFS image found.")
    else:
        print("Found SPIFFS image")

    i = input(f"Flashing:\n{factory_data}\nConfirm? [Y/n] ")
    if i.lower() != "y":
        print("Aborted.")
        quit()

    if not args.auto:
        # manual mode
        if not input("Put device to boot mode and press ENTER...") == "":
            print("Aborted.")
            quit()

        if not args.skip_efuse:
            print("Burning eFuses...")
            burn_efuses(args.port, args.baud, factory_data)
            if not input("Reset device to boot mode and press ENTER...") == "":
                print("Aborted.")
                quit()

        print("Flashing firmware...")
        flash_firmware(args.port, args.baud, firmware_path,
                       test_setup, spiffs_image_path)
        if args.series:
            append_inventory(inventory_path, factory_data)
        print("DONE.")
    else:
        # auto mode
        jig = HBMiniTestJig(args.jig_serial)
        jig.green_led_off()
        jig.reset_to_bootloader()
        sleep(0.5)
        if not args.skip_efuse:
            print("Burning eFuses...")
            burn_efuses(args.port, args.baud, factory_data)
            jig.reset_to_bootloader()
            sleep(0.5)
        print("Flashing firmware...")
        flash_firmware(args.port, args.baud, firmware_path,
                       test_setup, spiffs_image_path)
        sleep(0.5)
        jig.green_led_on()
        if args.series:
            append_inventory(inventory_path, factory_data)
        print("DONE.")

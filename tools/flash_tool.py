#!/usr/bin/env python

import os
import subprocess
from dataclasses import dataclass
from tempfile import NamedTemporaryFile, TemporaryDirectory
from time import sleep
import zipfile
import csv
import json
import argparse
from uuid import uuid4

NVS_GEN_PATH = "components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py"


@dataclass
class FactoryData:
    model_id: str
    hw_version: str
    serial: str
    random_id: str

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


def generate_random_id():
    return str(uuid4())[:6].upper()


def find_espidf():
    packages = os.path.join(os.path.expanduser("~"), ".platformio", "packages")
    assert os.path.exists(packages)
    for package in os.listdir(packages):
        if package == "framework-espidf":
            return os.path.join(packages, package)
    raise Exception("Could not find esp-idf platformio package")


def unzip_file(zip_path: str, extract_dir: str):
    with zipfile.ZipFile(zip_path, 'r') as zip_ref:
        zip_ref.extractall(extract_dir)


def load_partition_table(csv_path: str):
    data_dict = {}
    with open(csv_path, 'r') as file:
        csv_reader = csv.reader(file)
        next(csv_reader)
        next(csv_reader)
        for row in csv_reader:
            name = row[0].strip()
            data_type = row[1].strip()
            sub_type = row[2].strip()
            offset = row[3].strip()
            size = row[4].strip()
            if "K" in size:
                size = hex(int(size.replace("K", "")) * 1024)
            elif "M" in size:
                size = hex(int(size.replace("M", "")) * 1024 * 1024)
            flags = row[5].strip()
            data_dict[name] = {
                'Type': data_type,
                'SubType': sub_type,
                'Offset': offset,
                'Size': size,
                'Flags': flags
            }
    return data_dict


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


def burn_efuses(factory_data: FactoryData, port: str):
    factory_data = encode_factory_data(factory_data)
    with NamedTemporaryFile("wb") as f:
        f.write(factory_data)
        f.flush()
        tokens = ["espefuse.py", "--port", port, "--do-not-confirm",
                  "burn_block_data", "BLOCK3", f.name]
        try:
            subprocess.run(tokens, check=True, capture_output=True, text=True)
        except subprocess.CalledProcessError as e:
            print(e.stdout)
            quit()


def flash_firmware(port: str, fw_zip_path: str, test_setup: FactoryTestSetup, spiffs_image_path: str = None):
    with TemporaryDirectory() as tmp_dir:
        # unzip firmware
        unzip_file(fw_zip_path, tmp_dir)
        partitions = load_partition_table(
            os.path.join(tmp_dir, "partitions.csv"))

        # generate NVS image with test setup
        nvs_path = os.path.join(tmp_dir, "nvs.bin")
        generate_nvs(nvs_path, partitions["nvs"]
                     ["Size"], test_setup)

        tokens = ["esptool.py", "--port", port, "--after", "no_reset", "write_flash",
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

    parser.add_argument("port", type=str, help="Serial port")
    parser.add_argument("fw_dir", type=str,
                        help="Directory containing firmware files")

    parser.add_argument("--auto", action="store_true",
                        help="Auto mode to use with test jig")
    parser.add_argument("--jig_serial", type=str,
                        help="Serial number of test jig")

    parser.add_argument("--series", action="store_true",
                        help="If flashing multiple devices in series")
    parser.add_argument("--model_id", type=str,
                        required=True, help="Model ID of devices")
    parser.add_argument("--hw_version", type=str,
                        required=True, help="Hardware version of devices")
    parser.add_argument("--serial", type=str, required=True,
                        help="Serial number of device. If 'series', this is the first serial number.")
    parser.add_argument("--random_id", type=str, help="Random ID of device")

    args = parser.parse_args()

    if args.auto and args.jig_serial is None:
        parser.error("--jig_serial is required in auto mode")

    if args.series and args.random_id:
        parser.error("--random_id is not allowed in series mode")

    if not os.path.isdir(args.fw_dir):
        parser.error("Invalid firmware directory")

    # data check
    if len(args.model_id) != 2:
        parser.error("Invalid model ID")
    if len(args.hw_version) != 3:
        parser.error("Invalid hardware version")
    if len(args.serial) != 8:
        parser.error("Invalid serial number")
    if args.random_id and len(args.random_id) != 6:
        parser.error("Invalid random ID")

    # load firmware
    firmware_path = os.path.join(args.fw_dir, "firmware.zip")
    if not os.path.isfile(firmware_path):
        parser.error("Missing 'firmware.zip'")

    # load test setup
    test_setup_path = os.path.join(args.fw_dir, "test_setup.json")
    if os.path.isfile(test_setup_path):
        with open(test_setup_path, "r") as f:
            test_setup = FactoryTestSetup.from_json(f.read())
    else:
        parser.error("Missing 'test_setup.json'")

    # load factory data
    if args.random_id:
        random_id = args.random_id
    else:
        random_id = generate_random_id()
    factory_data = FactoryData(args.model_id, args.hw_version,
                               args.serial, random_id)

    # load spiffs image
    spiffs_image_path = os.path.join(args.fw_dir, "spiffs.bin")
    if not os.path.isfile(spiffs_image_path):
        spiffs_image_path = None
        print("No SPIFFS image found.")

    i = input(f"Flashing:\n{factory_data}\nConfirm? [y/N] ")
    if i.lower() != "y":
        print("Aborted.")
        quit()

    if not args.auto:
        if (input("Put device to boot mode and press ENTER...")) == "":
            print("Burning eFuses...")
            # burn_efuses(factory_data, args.port)
            print("Flashing firmware...")
            flash_firmware(args.port, firmware_path,
                           test_setup, spiffs_image_path)
            print("Done.")
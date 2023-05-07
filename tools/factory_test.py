#!/usr/bin/env python

from dataclasses import dataclass
import json
from time import sleep
import argparse
import inspect
import sys
import paho.mqtt.client as mqtt

base_topic = "homebuttons-factory"
devices_topic = f"{base_topic}/devices"


def split_topic(topic):
    return ("/".join(topic.split("/")[0:-1]), topic.split("/")[-1])


def join_topic(*args):
    return "/".join(args)


def json_serializable(cls):
    """A decorator that adds to_json() and from_json() methods to a dataclass.
    Supports nested dataclasses."""

    def to_json(self, indent=None):
        def serialize(data):
            if isinstance(data, dict):
                return {key: serialize(value) for key, value in data.items()}
            elif isinstance(data, list):
                return [serialize(item) for item in data]
            elif hasattr(data, "to_json"):
                return json.loads(data.to_json())
            else:
                return data

        return json.dumps(serialize(self.__dict__), indent=indent)

    @classmethod
    def from_json(cls, json_str):
        data = json.loads(json_str)

        # Get all classes in the module
        all_classes = inspect.getmembers(
            sys.modules[__name__], inspect.isclass)

        for key, value in data.items():
            # Check if the value is a dictionary, indicating a nested dataclass
            if isinstance(value, dict):
                # Find the corresponding class by name
                dataclass_cls = next(
                    (c for name, c in all_classes if name == key.capitalize()), None
                )
                if dataclass_cls:
                    data[key] = dataclass_cls.from_json(json.dumps(value))

        return cls(**data)

    cls.to_json = to_json
    cls.from_json = from_json

    return cls


@json_serializable
@dataclass
class TestSpec:
    description: str
    target_model_id: str
    parameters: dict


@json_serializable
@dataclass
class Device:
    serial: str
    random_id: str
    model_id: str
    fw_version: str
    hw_version: str


@json_serializable
@dataclass
class TestResult:
    device: Device
    passed: bool
    parameters: dict


class TestRunner:

    def __init__(self, out_file: str = None):
        self.mqtt_client = mqtt.Client()

        self.out_file = out_file

        self.test_spec: TestSpec = None
        self.devices: list[Device] = []

        self.mqtt_client.on_connect = self.on_connect
        self.mqtt_client.on_message = self.on_message

    def connect(self, server: str, port: int, username: str = None, password: str = None):
        if username is not None and password is not None:
            self.mqtt_client.username_pw_set(username, password)
        self.mqtt_client.connect(server, port, 60)
        self.mqtt_client.loop_start()

    def run_forever(self):
        while True:
            sleep(1)

    def on_connect(self, client, userdata, flags, rc):
        print(f"MQTT connected with result code {rc}")
        client.subscribe(f"{base_topic}/#")

    def on_message(self, client, userdata, msg):
        if msg.payload == b"":
            return

        # Device discovery
        if split_topic(msg.topic)[0] == devices_topic:
            try:
                device = Device.from_json(msg.payload)
            except (TypeError, json.JSONDecodeError):
                print(f"Received invalid device JSON: {msg.payload}")
                return
            if split_topic(msg.topic)[1] == device.serial:
                if device.serial not in [d.serial for d in self.devices]:
                    if device.model_id == self.test_spec.target_model_id:
                        self.devices.append(device)
                        print(f"Discovered device: {device.serial}")
                        self.start_test(device)
                    else:
                        print(
                            f"Discovered device with unsupported model ID: {device.model_id} (expected {self.test_spec.target_model_id})")
                else:
                    print(f"Device already discovered: {device.serial}")
            else:
                print(
                    f"Topic {msg.topic} does not match device serial {device.serial}")
            return

        # Test result
        if split_topic(msg.topic)[1] == "test_result":
            try:
                test_result = TestResult.from_json(msg.payload)
            except (TypeError, json.JSONDecodeError):
                print("Invalid test result JSON")
                return
            if split_topic(split_topic(msg.topic)[0])[1] != test_result.device.serial:
                print(
                    f"Topic {msg.topic} does not match device serial {test_result.device.serial}")
                return
            for device in self.devices:
                if device.serial == test_result.device.serial:
                    if test_result.passed:
                        print(f"Test PASSED for device: {device.serial}")
                    else:
                        print(f"Test FAILED for device: {device.serial}")
                    if self.out_file:
                        with open(self.out_file, "a") as f:
                            f.write(
                                f"#### TEST RESULT - DEVICE: {device.serial} ####\n")
                            f.write(test_result.to_json(indent=2) + "\n\n")
                    self.devices.remove(device)
                    break
            else:
                print(
                    f"Received test result for unknown device: {test_result.device.serial}")
            return

    def load_test_spec(self, spec: TestSpec):
        self.test_spec = spec

    def start_test(self, device: Device):
        if not self.test_spec:
            print("Failed to start test: no test spec loaded")
            return
        self.mqtt_client.publish(
            f"{devices_topic}/{device.serial}/test_start", self.test_spec.to_json())
        print(f"Started test on device: {device.serial}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Home Buttons Factory Test Tool")

    parser.add_argument("spec_file", type=str, help="Test spec JSON file")
    parser.add_argument("-o", "--output", type=str, help="Report output file")
    parser.add_argument("--mqtt_server", type=str, required=True)
    parser.add_argument("--mqtt_port", type=int, required=True)
    parser.add_argument("--mqtt_user", type=str, required=True)
    parser.add_argument("--mqtt_password", type=str, required=True)

    args = parser.parse_args()

    with open(args.spec_file, "r") as f:
        spec = f.read()

    test_spec = TestSpec.from_json(spec)
    test_runner = TestRunner(out_file=args.output)
    test_runner.load_test_spec(test_spec)
    print("Test spec loaded successfully")

    test_runner.connect(args.mqtt_server, args.mqtt_port,
                        args.mqtt_user, args.mqtt_password)

    test_runner.run_forever()

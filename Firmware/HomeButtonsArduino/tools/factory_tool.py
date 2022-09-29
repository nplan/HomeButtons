from dataclasses import dataclass
from serial import Serial
from datetime import date
from uuid import uuid4
import argparse

port = "/dev/cu.usbmodem01"
baud = 115200


@dataclass
class FactorySettings:
    serial_number: str = ""
    random_id: str = ""
    model_name: str = ""
    model_id: str = ""
    hw_version: str = ""

    def __repr__(self) -> str:
        return f"Factory settings:\n"\
            f"    serial_number: {self.serial_number}\n"\
            f"    random_id: {self.random_id}\n"\
            f"    model_name: {self.model_name}\n"\
            f"    model_id: {self.model_id}\n"\
            f"    hw_version: {self.hw_version}\n"


class FactoryHandler:

    def __init__(self) -> None:
        self.serial: Serial = None

    def run_hw_test(self) -> bool:
        print("Starting HW test...", end="", flush=True)
        self.serial.write(b"T\r")
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
        self.serial.write(b"S-" + serial_number.encode("ascii") + b"\r")
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
        self.serial.write(b"I-" + random_id.encode("ascii") + b"\r")
        ret = self.serial.readline().decode().strip()
        if ret == "OK":
            print("OK")
            return True
        else:
            print("FAIL")
            return False

    def set_model_name(self, model_name: str) -> bool:
        assert type(model_name) is str
        assert 0 < len(model_name) <= 20
        print("Setting model name...", end="", flush=True)
        self.serial.write(b"M-" + model_name.encode("ascii") + b"\r")
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
        self.serial.write(b"D-" + model_id.encode("ascii") + b"\r")
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
        self.serial.write(b"V-" + hw_version.encode("ascii") + b"\r")
        ret = self.serial.readline().decode().strip()
        if ret == "OK":
            print("OK")
            return True
        else:
            print("FAIL")
            return False

    def save_settings(self) -> bool:
        print("Saving settings...", end="", flush=True)
        self.serial.write(b"E\r")
        ret = self.serial.readline().decode().strip()
        if ret == "OK":
            print("OK")
            return True
        else:
            print("FAIL")
            return False

    def run_factory_setup(self, settings: FactorySettings):
        with Serial(port, baudrate=baud) as self.serial:
            if not self.set_hw_version(settings.hw_version):
                return
            if not self.run_hw_test():
                return
            if not self.set_serial_number(settings.serial_number):
                return
            if not self.set_random_id(settings.random_id):
                return
            if not self.set_model_name(settings.model_name):
                return
            if not self.set_model_id(settings.model_id):
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
    required.add_argument("--model_name", type=str, required=True)
    required.add_argument("--model_id", type=str, required=True)
    required.add_argument("--hw_version", type=str, required=True)

    args = parser.parse_args()

    settings = FactorySettings(
        serial_number=args.serial,
        random_id=args.random_id if args.random_id else str(uuid4())[
            :6].upper(),
        model_name=args.model_name,
        model_id=args.model_id,
        hw_version=args.hw_version
    )

    factory = FactoryHandler()
    factory.run_factory_setup(settings)

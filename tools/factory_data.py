from dataclasses import dataclass
from uuid import uuid4


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

    @classmethod
    def decode(cls, b: bytearray):
        serial = b[0:8].decode("utf-8")
        random_id = b[8:14].decode("utf-8")
        model_id = b[14:16].decode("utf-8")
        hw_version = b[16:19].decode("utf-8")
        return cls(model_id, hw_version, serial, random_id)

    def encode(self):
        serial = self.serial
        random_id = self.random_id
        model_id = self.model_id
        hw_version = self.hw_version

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
        assert self.decode(b) == self
        return b

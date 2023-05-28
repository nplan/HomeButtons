import time
import usb.core
import usb.util


class HBMiniTestJig:

    class DeviceNotFound(Exception):
        pass

    _PID = 0xea60
    _VID = 0x10c4

    # see AN571 page 18 for values
    _req = 0xFF  # vendor specific request
    _bm_req_type = 0x41  # write latch request (GPIO)
    _w_value = 0x37E1  # wValue for latch write

    # pins
    _gpio_rst = 0
    _gpio_boot = 1
    _gpio_led = 2

    # constructor finds device. If not found, prints all attached devices and raises exception

    def __init__(self, serial_number=None):
        self.serial_number = serial_number
        self._device = usb.core.find(
            idVendor=self._VID, idProduct=self._PID, custom_match=self.__find_by_serial(self.serial_number))
        if (self._device):
            # print("found device!")

            # set gpios to normal state
            self.set_gpio(self._gpio_rst, 1)
            self.set_gpio(self._gpio_boot, 1)
            self.set_gpio(self._gpio_led, 0)
        else:
            print("Device " + self.serial_number +
                  " not found. Devices attached:")
            self.find_devices()  # print all available serials
            raise self.DeviceNotFound("CP2104 device not found!")

    def reset(self):
        self.set_gpio(self._gpio_rst, 0)  # trigger reset
        time.sleep(10e-3)
        self.set_gpio(self._gpio_rst, 1)  # release reset

    def reset_to_bootloader(self):
        self.set_gpio(self._gpio_rst, 0)  # trigger reset
        time.sleep(10e-3)
        self.set_gpio(self._gpio_boot, 0)  # set boot pin
        time.sleep(100e-3)
        self.set_gpio(self._gpio_rst, 1)  # release reset
        time.sleep(100e-3)  # wait for boot
        self.set_gpio(self._gpio_boot, 1)  # return boot pin to normal state

    def green_led_on(self):
        self.set_gpio(self._gpio_led, True)

    def green_led_off(self):
        self.set_gpio(self._gpio_led, False)

    # finds serial numbers of all attached SiLabs CP2104 devices.
    def find_devices(self):
        devices = usb.core.find(
            find_all=True, idVendor=self._VID, idProduct=self._PID)
        if devices is None:
            raise ValueError('Device not found')
        for dev in devices:
            print(usb.util.get_string(dev, dev.iSerialNumber))

    def set_gpio(self, gpio, state):
        state = bool(state)
        if gpio < 0 or gpio > 7 or state < 0 or state > 1:
            raise ValueError("Invalid gpio or state input")
        byte1 = 1 << gpio
        byte2 = state << 8+gpio | byte1
        try:
            self._device.ctrl_transfer(
                self._bm_req_type, self._req, self._w_value, byte2, [])
        except:
            raise self.DeviceNotFound("CP2104 device not accessible!")

    def __find_by_serial(self, serial):
        def matcher(dd):
            try:
                return dd.serial_number == serial
            except:
                return False
        return matcher


# Testing section
if __name__ == '__main__':
    dut = HBMiniTestJig("02919B1A")
    # dut.find_devices()

    time.sleep(1)
    dut.reset_to_bootloader()
    time.sleep(2)
    dut.reset()

    while (True):
        dut.green_led_on()
        time.sleep(0.5)
        dut.green_led_off()
        time.sleep(0.5)

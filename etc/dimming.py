#! /usr/bin/env python3

from sys import argv
from i2c_puppet import I2CPuppet
from os import uname

if uname().sysname == 'Darwin' and uname().machine == 'arm64':
    import usb.backend.libusb1
    usbBackend = usb.backend.libusb1.get_backend(find_library=lambda x: "/opt/homebrew/lib/libusb-1.0.dylib")
else:
    usbBackend = None

def parseDelay(value):
    delay = float(value)
    if delay < 0.0 or delay > 127.5:
        raise IndexError("delay must be between 0.0 and 127.5 inclusive")
    else:
        return delay

def parseLevel(value):
    level = float(value)
    if level < 0.0 or level > 100.0:
        raise IndexError("level must be between 0.0 and 100.0 inclusive")
    else:
        return level / 100

def displayUsage():
    print(f"""
Usage:
    {argv[0]}
        Displays the current dimming delay in seconds and the level as a percentage in
        a human friendly format.

    {argv[0]} delay [value]
        Return or set the dimming delay in seconds. If 'value' is provided, it must be
        between 0.0 and 127.5 inclusive. The value will be rounded to the nearest half
        second. Setting this to 0 disables dimming of the keyboard backlight.

    {argv[0]} level [value]
        Return or set the brightness level when the keyboard backlight dims. If 'value'
        is provided, it must be between 0.0 and 100.0 inclusive.

    {argv[0]} delay level
        Sets both the delay and level at the same time. The same limits as described
        above apply to the arguments.
""")

if __name__ == "__main__":
    if len(argv) < 4:
        device = I2CPuppet(backend = usbBackend)
        if len(argv) == 1:
            print("Delay: {}s".format(device.dimmingDelay))
            print("Level: {:.2f}%".format(device.dimmingLevel * 100))
        elif argv[1] == "delay":
            if len(argv) == 3:
                device.dimmingDelay = parseDelay(argv[2])
            else:
                print(device.dimmingDelay)
        elif argv[1] == "level":
            if len(argv) == 3:
                device.dimmingLevel = parseLevel(argv[2])
            else:
                print(device.dimmingLevel)
        else:
            try:
                delay = parseDelay(argv[1])
                level = parseLevel(argv[2])
                device.dimmingDelay = delay
                device.dimmingLevel = level
            except ValueError:
                displayUsage()
    else:
        displayUsage()


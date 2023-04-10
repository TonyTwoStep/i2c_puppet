#! /usr/bin/env python3

from sys import argv
from i2c_puppet import I2CPuppet, _REG_RST
from usb import USBError # pyusb module
from os import uname

if uname().sysname == 'Darwin' and uname().machine == 'arm64':
    import usb.backend.libusb1
    usbBackend = usb.backend.libusb1.get_backend(find_library=lambda x: "/opt/homebrew/lib/libusb-1.0.dylib")
else:
    usbBackend = None

if __name__ == "__main__":
    if (len(argv) == 1) or ((len(argv) == 2) and (argv[1] == "uf2")):
        device = I2CPuppet(backend = usbBackend)
        UF2Boot = ((len(argv) == 2) and (argv[1] == "uf2"))

        if UF2Boot:
            device._write_register(_REG_RST, 0x99)
        else:
            try:
                device._read_register(_REG_RST)
            except USBError:
                pass

    else:
        print(f"""
Usage: {argv[0]} [uf2]

Resets the BBQ2KBD. If the optional 'uf2' argument is provided, resets into
programer mode allowing you to mount 'RPI-RP2' to load new uf2 files.
""")


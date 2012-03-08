This example program shows you how to use libusb to communicate
with an XTAG2. It implements the boot loader protocol [1] that enables
you to send firmware over USB to the XTAG2, and execute that firmware
on the XTAG2 itself.

Compile the example program as follows (on a Mac, use the appropriate
directories for other Windows and Linux):

  g++ -m32 device_if.cpp main.cpp device_serial_number.cpp -Ilibusb/Mac libusb/Mac/libusb-1.0.dylib 

The firmware that will be loaded is specified in l1_jtag.h (in this
example the JTAG firmware that xgdb uses). You can replace the
contents of l1_jtag.h with other binary code. In order to extract
a binary from a .xe file, you can use xobjdump as follows:

  xobjdump --strip --split blah.xe

This will create a file image_n0c0.bin that you can view or manipulate
using an appropriate binary viewer (eg, od -t x2 under MacOSX/Unix)


[1] "USB Bootloader Description and Standards", http://www.xmos.com/

# Dump out flash from RTL bootloader... very slowly!
#====================================================
# Author: Paul Banks [https://paulbanks.org/]
#

import serial
import struct

s = serial.Serial("/dev/ttyUSB0", 38400)

def doit(fOut):

    # Get a couple of prompts for sanity
    s.write(b"\n")
    s.read_until(b"<RealTek>")
    s.write(b"\n")
    s.read_until(b"<RealTek>")

    print("Starting...")

    step = 0x100
    assert(step%4==0)
    for flash_addr in range(0, 16*1024*1024, step):

        s.write(b"FLR 80000000 %X %d\n" % (flash_addr, step))
        print(s.read_until(b"--> "))
        s.write(b"y\r")
        print(s.read_until(b"<RealTek>"))

        s.write(b"DW 80000000 %d\n" % (step/4))

        data = s.read_until(b"<RealTek>").decode("utf-8").split("\n\r")
        for l in data:
            parts = l.split("\t")
            for p in parts[1:]:
                fOut.write(struct.pack(">I", int(p, 16)))
            if len(parts) != 5:
                print(l)

with open("spiflash.bin","wb") as fOut:
    doit(fOut)


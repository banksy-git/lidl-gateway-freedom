# Dump out flash from RTL bootloader... very slowly!
#====================================================
# Author: Paul Banks [https://paulbanks.org/]
#

import serial
import struct
import argparse


def doit(s, fOut, start_addr=0, end_addr=16*1024*1024):

    # Get a couple of prompts for sanity
    s.write(b"\n")
    s.read_until(b"<RealTek>")
    s.write(b"\n")
    s.read_until(b"<RealTek>")

    print("Starting...")

    step = 0x100
    assert(step%4==0)
    for flash_addr in range(start_addr, end_addr, step):

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


if __name__=="__main__":

    parser = argparse.ArgumentParser("RTL Flash Dumper")
    parser.add_argument("--serial-port", type=str,
                        help="Serial port device - e.g. /dev/ttyUSB0")
    parser.add_argument("--output-file", type=str,
                        help="Path to file to save dump into")
    parser.add_argument("--start-addr", type=str, help="Start address",
                        default="0x0")
    parser.add_argument("--end-addr", type=str, help="End address",
                        default=hex(16*1024*1024))

    args = parser.parse_args()

    s = serial.Serial(args.serial_port, 38400)
    start_addr = int(args.start_addr, 0)
    end_addr = int(args.end_addr, 0)

    with open(args.output_file,"wb") as fOut:
        doit(s, fOut, start_addr, end_addr)


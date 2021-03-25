#!/usr/bin/python

import argparse
import socket
import sys

try:
    from xmodem import XMODEM
except:
    print("ERROR: You need to install the Python XMODEM library. (pip install xmodem)",
          file=sys.stderr)
    sys.exit(-1)

OOB_HW_FLOW_OFF = b"\x10"
OOB_HW_FLOW_ON = b"\x11"

def upgrade_ncp(ip, port, firmware_stream):

    s = socket.create_connection((ip, port))
    s.settimeout(5)
    s.send(OOB_HW_FLOW_OFF, socket.MSG_OOB)

    print("Entering upload mode...")

    s.send(b"1\n") # Bootloader command to begin upload
    while s.recv(1) != b"\x00":
        pass

    print("Waiting for XMODEM...")
    while s.recv(1) != b"C":
        pass

    print("Uploading firmware...")

    def getc(size, timeout=1):
        c = s.recv(size)
        return c

    def putc(data, timeout=1):
        return s.send(data)

    modem = XMODEM(getc, putc)
    modem.send(firmware_stream)

    print("Upload complete. Starting new firmware...")
    s.send(b"2\n") # Bootloader command to start new firmware

    s.sendall(OOB_HW_FLOW_ON, socket.MSG_OOB)

    s.close()

    print("Done")


parser = argparse.ArgumentParser("Upgrade Lidl NCP through SerialGateway")
parser.add_argument("ip", type=str, help="IP address of device")
parser.add_argument("firmware", type=argparse.FileType('rb'),
                    help="Filename of firmware")
parser.add_argument("--port", type=int, help="TCP port of serial gateway",
                    default=8888)
parser.add_argument("--no-confirm", action='store_true',
                    help="Skip all the warnings.")

args = parser.parse_args()

if args.no_confirm != True:
    print("""
WARNING: THIS TOOL COMES WITH NO WARRANTY. USE AT YOUR OWN RISK.

This will replace the firmware on your ZigBee Network Co-Processor

 * Ensure your firmware file comes from a trusted and tested source.

 * Failed upgrades may require special hardware tools to recover.

 * After this operation you should not use this gateway on the original cloud
   service.

To begin, first place device in bootloader mode using Bellows, e.g.:

    bellows -d socket://%s:%d bootloader

And then enter the word: UPGRADE below""" % (args.ip, args.port))

    if input(">").lower().strip()!="upgrade":
        print("Not upgrading.")
        sys.exit()

print("Attempting upgrade...")

upgrade_ncp(args.ip, args.port, args.firmware)



# RTL RootFS tool
#=================
# Author: Paul Banks [https://paulbanks.org/]
#

import os
import sys
import struct
import argparse

SQUASHFS_MAGIC_BE = 0x68737173
SIZE_OF_SQFS_SUPER_BLOCK = 640

def cmd_check(args):

    file_size = os.path.getsize(args.rootfs_flash_image)
    with open(args.rootfs_flash_image, "rb") as f:

        header_type = ">IIIIIH"

        (magic, inode_count, modification_time,
         block_size, fragment_entry_count, compression_id) = struct.unpack(
                 header_type, f.read(struct.calcsize(header_type)))
    
        if magic != SQUASHFS_MAGIC_BE:
            raise RuntimeError("Doesn't appear to be a SquashFS.")

        # RTL have repurposed this header to be the size for checksum purposes
        rtl_size = modification_time + SIZE_OF_SQFS_SUPER_BLOCK + 2
        if rtl_size > file_size:
            raise RuntimeError("Not an RTL Squash filesystem")

        f.seek(0) 
        checksum = 0
        for _ in range((rtl_size//2)):
            word, = struct.unpack(">H", f.read(2))
            checksum += word
            checksum &= 0xFFFF
        
        if checksum != 0:
            print("Checksum failed")
        else:
            print("Checksum passed")

def _checksum_buffer(b, checksum=0):
    for i in range(0, len(b), 2):
        word, = struct.unpack(">H", b[i:i+2])
        checksum += word
        checksum &= 0xFFFF
    return checksum

def cmd_build(args):

    squashfs_size = os.path.getsize(args.squashfs_image) \
            - SIZE_OF_SQFS_SUPER_BLOCK 
    if (squashfs_size % 2) != 0:
        squashfs_size += 1
    with open(args.squashfs_image, "rb") as fIn:

        header_prefix = fIn.read(8)
        magic, _ = struct.unpack(">II", header_prefix)
        if magic != SQUASHFS_MAGIC_BE:
            raise RuntimeError("Doesn't appear to be a SquashFS.")

        fIn.read(4) # Skip modification time from original

        header_prefix += struct.pack(">I", squashfs_size)

        with open(args.rootfs_image, "wb") as fOut:
            checksum = _checksum_buffer(header_prefix)
            fOut.write(header_prefix)

            while True:
                b = fIn.read(1024)
                if b==b"":
                    break
                if (len(b) % 2) != 0:
                    b += b"\0"
                checksum = _checksum_buffer(b, checksum)
                fOut.write(b)

            fOut.write(struct.pack(">H", (0x10000 - checksum)&0xFFFF))


if __name__=="__main__":

    parser = argparse.ArgumentParser("RTL RootFS tool")
    subparsers = parser.add_subparsers()

    p = subparsers.add_parser("check", help="Check integrity of RTL RootFS image")
    p.add_argument("rootfs_flash_image", type=str)
    p.set_defaults(func=cmd_check)

    p = subparsers.add_parser("build", help="Build an RTL RootFS image from a SquashFS")
    p.add_argument("squashfs_image", type=str)
    p.add_argument("rootfs_image", type=str)
    p.set_defaults(func=cmd_build)

    args = parser.parse_args()
    if getattr(args, "func", None):
        args.func(args)
    else:
        print("OOPS: Need command. Run %s -h for help." % sys.argv[0])
        sys.exit(1)





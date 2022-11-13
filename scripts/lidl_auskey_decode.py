# Lidl Silvercrest SmartKey Root Decoder v0.3
#=============================================
# By Banksy
#
# PREREQ's:
#   Python 3
#   Install cryptography Python library
#   You need to have opened your device and wired up a serial port to it.
#
# INSTRUCTIONS:
# 
#  1) Attach TTY3v3 serial port to device.
#  2) Load terminal program, set params 38400, 8, N, 1 - NO FLOW CONTROL
#  3) Power on device and at same time, press ESC a few times.
#  4) You should end up at a <RealTek> prompt
#  5) Press ENTER
#  6) Type "FLR 80000000 401802 16" followed by enter to read the KEK from
#     flash into memory.
#  7) Type "dw 80000000 4" to display the KEK.
#  8) Type "FLR 80000000 402002 32" followed by enter to read the encoded 
#     auskey from flash into RAM
#  9) Type "dw 80000000 8" to display the encrypted AUSKEY
# 10) Run this script
# 11) Paste the single line KEK output from step 7
# 12) Then paste the two lines of AUSKEY output from step 9
#
# Enjoy root.
#

import sys
if sys.version_info[0] < 3:
    raise RuntimeError("Python 3 is required to run this script!")
from binascii import unhexlify
import struct
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend

def _aschar(b):
    return struct.unpack("b", bytes([b & 0xFF]))[0]

def _decode_kek(a):
    if len(a) != 16:
        raise ValueError("KEK incorrect length. Should be 16 was %d" % len(a))
    kek = []
    for b in a:
        c1 = _aschar(a[0] * b)
        c2 = _aschar((a[0] * b) // 0x5d)
        kek += [_aschar(c1 + c2 * -0x5d + ord('!'))]
    return bytes(kek)

def _get_bytes(a):
    a = a.replace(" ", "").replace("\t", "").split(":")
    return unhexlify(a[0] if len(a)==1 else a[1])

kek = _decode_kek(_get_bytes(input("Enter KEK hex string line>")))

encoded_key = b""
for n in range(2):
    a = input("Encoded aus-key as hex string line %d>" % (n+1))
    encoded_key += _get_bytes(a)

cipher = Cipher(algorithms.AES(kek), modes.ECB(), default_backend())
decryptor = cipher.decryptor()
auskey = decryptor.update(encoded_key) + decryptor.finalize()

print("Auskey:", auskey.decode("ascii"))
print("Root password:", auskey[-8:].decode("ascii"))

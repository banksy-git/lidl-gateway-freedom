#!/bin/bash
GATEWAY_HOST="$1"
SSH_PORT="$2"
FIRMWARE_FILE="$4"
EZSP_VERSION="$3"
CONFIGURATION_FRAME="\x00\x42\x21\xA8\x53\xDD\x4F\x7E"

if [ $EZSP_VERSION = "V8" ]; then
  CONFIGURATION_FRAME="\x00\x42\x21\xA8\x5C\x2C\xA0\x7E"
fi

cat sx | ssh -p${SSH_PORT} root@${GATEWAY_HOST} "cat > /tmp/sx"
cat ${FIRMWARE_FILE} | ssh -p${SSH_PORT} root@${GATEWAY_HOST} "cat > /tmp/firmware.gbl"

ssh -p${SSH_PORT} root@${GATEWAY_HOST} "
chmod +x /tmp/sx
killall -q serialgateway
stty -F /dev/ttyS1 115200 cs8 -cstopb -parenb -ixon crtscts raw
echo -en '\x1A\xC0\x38\xBC\x7E' > /dev/ttyS1
echo -en '$CONFIGURATION_FRAME' > /dev/ttyS1
echo -en '\x81\x60\x59\x7E' > /dev/ttyS1
echo -en '\x7D\x31\x43\x21\x27\x55\x6E\x90\x7E' > /dev/ttyS1
stty -F /dev/ttyS1 115200 cs8 -cstopb -parenb -ixon -crtscts raw
echo -e '1' > /dev/ttyS1
/tmp/sx /tmp/firmware.gbl < /dev/ttyS1 > /dev/ttyS1
reboot
"

echo "Successfully flashed new EZSP firmware! The device will now reboot."

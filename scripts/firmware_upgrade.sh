#!/bin/bash
GATEWAY_HOST="$1"
SSH_PORT="22"
SOCAT_PORT="8888"
FIRMWARE_FILE="$2"

if ! bellows -d socket://${GATEWAY_HOST}:${SOCAT_PORT} bootloader | grep -q 'bootloader launched successfully'; then
  echo "Failed to launch bootloader via Bellows."
  exit 1
fi

cat sx | ssh -p${SSH_PORT} root@${GATEWAY_HOST} "cat > /tmp/sx"
cat ${FIRMWARE_FILE} | ssh -p${SSH_PORT} root@${GATEWAY_HOST} "cat > /tmp/firmware.gbl"

ssh -p${SSH_PORT} root@${GATEWAY_HOST} "
chmod +x /tmp/sx
killall serialgateway
stty -F /dev/ttyS1 115200 cs8 -cstopb -parenb -ixon -crtscts raw
echo -e '1' > /dev/ttyS1
/tmp/sx /tmp/firmware.gbl < /dev/ttyS1 > /dev/ttyS1
reboot
"

echo "Successfully flashed new EZSP firmware! The device will now reboot."

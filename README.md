# free-your-silvercrest
Freeing the Silvercrest (Lidl/Tuya) Smart Home Gateway from the cloud

A collection of scripts/programs for freeing your Silvercrest
Gateway from the cloud.

More information available here: 

* https://paulbanks.org/projects/lidl-zigbee/

## Upgrade firmware of the Zigbee module
By default, the Lidl gateway ships with EmberZNet version 6.5.0.0 which is quite old. It is possible to upgrade this to a newer version using a script provided in this repository.

The latest firmware, 6.7.8.0, is available here: https://github.com/grobasoz/zigbee-firmware/raw/master/EFR32%20Series%201/EFR32MG1B-256k/NCP/NCP_UHW_MG1B232_678_PA0-PA1-PB11_PA5-PA4.gbl

The procedure is as follows:

1. Make sure you have followed the initial setup to gain root SSH access on the device.
2. Download the latest firmware and place it somewhere convenient.
3. Execute the `firmware_upgrade.sh` script from within the scripts folder. Use the IP address of the gateway, the SSH port, and the filename of the firmware file as parameters, like this: `./firmware_upgrade.sh 192.168.1.4 2222 ncp_firmware.gbl`.
4. When prompted, enter the SSH password for the gateway. This occurs three times.
5. The new firmware should now be flashed, and the device will reboot. You can check the version with bellows: `bellows -d socket://GATEWAY_IP:8888 info`.
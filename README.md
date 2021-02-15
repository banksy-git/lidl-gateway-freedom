# free-your-silvercrest
Freeing the Silvercrest (Lidl/Tuya) Smart Home Gateway from the cloud

A collection of scripts/programs for freeing your Silvercrest
Gateway from the cloud.

More information available here: 

* https://paulbanks.org/projects/lidl-zigbee/

## Upgrade firmware of the Zigbee module
By default, the Lidl gateway ships with EmberZNet version 6.5.0.0 which is quite old. It is possible to upgrade this to a newer version using a script provided in this repository.

The newest firmware, 6.7.8.0, is available here: https://github.com/grobasoz/zigbee-firmware/raw/master/EFR32%20Series%201/EFR32MG1B-256k/NCP/NCP_UHW_MG1B232_678_PA0-PA1-PB11_PA5-PA4.gbl

For this to work, you need to have completed the initial setup, including installing `serialgateway` on the device.

The procedure is as follows:

1. Install bellows, which is a Python 3 library for interacting with EZSP. This should be a simple matter of running `pip install bellows`.
2. Execute the `firmware_upgrade.sh` script from within the scripts folder. Use the IP address of the gateway and filename of the firmware file as parameters, like this: `./firmware_upgrade.sh 192.168.1.4 ncp_firmware.gbl`.
3. When prompted, enter the SSH password for the gateway. This occurs three times.
4. The new firmware should now be flashed, and the device will reboot. You can check the version with bellows: `bellows -d socket://GATEWAY_IP:8888 info`.
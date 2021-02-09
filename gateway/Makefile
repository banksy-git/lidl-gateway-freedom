CC=/mnt/tmp/opt/realtek/rtl819x/toolchain/rsdk-4.4.7-4181-EB-2.6.30-0.9.30-m32u-140129/bin/mips-linux-gcc

build:
		$(CC) -static main.c -o serialgateway

upload: build
		cat serialgateway | ssh root@192.168.1.86 "cat >/tuya/serialgateway"

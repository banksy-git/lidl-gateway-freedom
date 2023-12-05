CC=/mnt/tmp/opt/realtek/rtl819x/toolchain/rsdk-4.4.7-4181-EB-2.6.30-0.9.30-m32u-140129/bin/mips-linux-gcc

build:
		$(CC) -static mdns.c -o mdns
		md5sum mdns > md5sum

upload: build
		cat mdns | ssh root@192.168.1.86 "cat >/tuya/mdns"

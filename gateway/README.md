Serial Gateway
==============
Author: Paul Banks [https://paulbanks.org]

A simple program to make the Zigbee radio module on /dev/ttyS1 
available over TCP/IP.

Building
--------

You need an appropriate toolchain. The Makefile gives a good hint!

Running
-------

After copying to the gateway, just start it. It sets up the serial
port with the correct parameters and listens for incoming connections 
on TCP/8888 and forwards the data to and from the serial port.

Only one connection at a time is supported. (Subsequent connections
will disconnect the previous ones to maintain integrity.)


# PalmGET
PalmOS4 application to GET an HTTP page.

## Requirements

* PalmOS SDK 4
* GCC for Palm
* PiLRC

## Setting up the SDK

Grab https://github.com/jichu4n/palm-os-sdk, we're interested in sdk-4. There's noting special needed here other than having it on disk.

## Setting up GCC

Grab https://github.com/hezi/prc-tools-remix. Before following the build from source directions, I needed to change `-lterminfo` to `-ltinfo` in `gdb-5.3/gdb/configure.in b/gdb-5.3/gdb/configure.in`. Otherwise the compile from source directions worked fine. I didn't bother with the binary tarball or unpacking to system, I run it from the build folder.

GCC needs to have the 'specs' for the SDK added. There's a tool for this, but it doesn't seem to run correctly and outputs nothing to the specified file, so redirect the output to a second file. Save that file somewhere, then strip out everything before `*cpp_sdk_5r4:` (or whatever the first line starting with * is).

```
cd ./build/tools
./palmdev-prep /PATH/to/palm-os-sdk/ --dump-specs /tmp/specs > /path/to/specs2
```

Note change `/path/to/` to whatever folder you're working out of. `$PATH` needs to be updated for the locations of GCC

```
export PATH="/path/to/prc-tools-remix/dist/usr/lib/gcc-lib/m68k-palmos/2.95.3-kgpd:$PATH"
export PATH="/path/to/prc-tools-remix/dist/usr/m68k-palmos/bin:$PATH"
export PATH="/path/to/prc-tools-remix/dist/usr/bin:$PATH"
```

## Setting up PiLRC

PiLRC is needed for encoding resources into the PRC file. Grab it from https://sourceforge.net/projects/pilrc/files/pilrc/3.2/, I used the .tar.gz. Compiling is straightforward, from the pilrc-3.2 folder, run `./unix/configure` followed by make and it's done. `pilrc` can be run from the folder no problem, and no patches needed to compile on a modern system. Just add it to the `$PATH`.

```
export PATH="/path/to/pilrc-3.2:$PATH"
```

## Compiling

To compile:

```
m68k-palmos-gcc PalmGET.c -o PalmGET.o -lNetSocket -specs=/path/to/specs2 -palmos4 \
    -B/path/to/prc-tools-remix/dist/usr/lib/gcc-lib/ \
    -B/path/to/prc-tools-remix/dist/usr/m68k-palmos/lib/ \
    -B/path/to/palm-os-sdk/sdk-4/lib/m68k-palmos-coff/
m68k-palmos-obj-res PalmGET.o
pilrc *.rcp
build-prc PalmGET.prc "PalmGET" PGET *.grc *.bin
rm *.grc *.bin
```

I'm not 100% sure what all of this is doing, but it does it. The first arguments to gcc are the standard source and object file type arguments. `-lNetSocket` links against the PalmOS networking library, the remaining arguments are for linking against the PalmOS system libraries.

## Network configuration

You'll need an internet connection setup on the PalmPilot. The easiest way to do that is via PPP over Serial/USB (the commands below assume `/dev/ttyUSB0` for the serial device). You'll need `ppp` support in the kernel, as well as `netfilter`.

Enable IPv4 forwarding, then run pppd. (Some firewall modifications may be required, this is distro dependent, but if you're using ufw, then `ufw allow from 10.0.0.2` will allow incoming traffic from the PalmPilot.)

```
# echo 1 > /proc/sys/net/ipv4/ip_forward
# iptables -t nat -I POSTROUTING -s 10.0.0.0/8 -o eth0 -j MASQUERADE
# pppd /dev/ttyUSB0 115200 10.0.0.1:10.0.0.2 proxyarp passive silent noauth local persist nodetach
```

This is set so the PC has an IP of 10.0.0.1 and the PalmPilot an IP of 10.0.0.2, which is given internet access through NAT (configured via iptables). DHCP is annoying to setup, so you can use a static IP address configuration on the PalmPilot with something like 1.1.1.1 or 8.8.8.8 setup for the DNS.

To setup the PalmPilot, in the Connection section of Prefs, make sure Cradle/Cable is selected and that in Details the Speed matches the pppd command (or vice versa). The IP configuration on the Palm side is set in Prefs in the Network section. Select the Service Unix, then hit Details. Uncheck Query DNS to enter DNS servers, and untick IP Address to enter the static IP. Hit Script and change the drop down in the first line to End. Hit Connect and make sure pppd is running and you should be online. MergicPing is useful for testing that networking is setup correctly.

## Running

Just send the PRC to the PalmPilot using `pilot-xfer` from https://github.com/jichu4n/pilot-link (this is probably in your distro's repo), then tap on the screen to run. Press "GET" to fetch the preprogrammed URL. Pressing "GET" will start the network connection.

`pilot-xfer -p /dev/ttyUSB0 -i PalmGET.prc`

To change the page being displayed, edit the `#define`s, with the URI (without hostname) in `TO_GET` and the hostname in `REMOTE_HOST`.

Note, there is a risk of crashing with a Fatal Exception requiring a soft or hard reset. Use at your own risk.

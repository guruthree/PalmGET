# PalmGET
PalmOS4 application to GET an HTTP page.

## Requirements

PalmOS SDK 4
GCC for Palm

## Setting up the SDK

Grab https://github.com/jichu4n/palm-os-sdk, we're interested in sdk-4. There's noting special needed here other than having it on disk.

## Setting up GCC

Grab https://github.com/hezi/prc-tools-remix. Before following the build from source directions, I needed to change `-lterminfo` to `-ltinfo` in `gdb-5.3/gdb/configure.in b/gdb-5.3/gdb/configure.in`. Otherwise the compile from source directions worked fine. I didn't bother with the binary tarball or unpacking to system, I run it from the build folder.

GCC needs to have the 'specs' for the SDK added. There's a tool for this, but it doesn't seem to run correctly and outputs nothing to the specified file, so redirect the output to a second file. Save that file somewhere, then strip out everything before `*cpp_sdk_5r4:` (or whatever the firt line starting with * is).

```
cd ./build/tools
./palmdev-prep /PATH/to/palm-os-sdk/ --dump-specs /tmp/specs > /path/to/specs2
```

`$PATH` needs to be updated for the locations of GCC

````
export PATH="/path/to/prc-tools-remix/dist/usr/lib/gcc-lib/m68k-palmos/2.95.3-kgpd:$PATH"
export PATH="/path/to/prc-tools-remix/dist/usr/m68k-palmos/bin:$PATH"
export PATH="/path/to/prc-tools-remix/dist/usr/bin:$PATH"
```

## Compiling

To compile:

```
m68k-palmos-gcc PalmGET.c -o PalmGET -lNetSocket -specs=/path/to/specs2 -palmos4 \
    -B/path/to/prc-tools-remix/dist/usr/lib/gcc-lib/ \
    -B/path/to/prc-tools-remix/dist/usr/m68k-palmos/lib/ \
    -B/path/to/palm-os-sdk/sdk-4/lib/m68k-palmos-coff/
m68k-palmos-obj-res PalmGET
build-prc PalmGET.prc "PalmGET" WRLD *.PalmGET.grc
```

I'm not 100% sure what all of this is doing, but it does it.

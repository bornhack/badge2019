# Bornhack Badge 2019

## About the hardware

Like the previous two years this years badge is again using the SiLabs Happy Gecko
microcontroller. This year it's sporting a 240x240 pixel screen with 6x6x6-bit colour
resolution, a MicroSD card reader and some basic IR communication.

We have separated the hardware design files from the code and put them in the
[hardware branch][hardware] for you to have a look at and build on.

You can download the microcontroller [reference manual][manual] and
[datasheet][] directly from [SiliconLabs][silabs].

[hardware]: https://github.com/bornhack/badge2019/tree/hardware
[silabs]: https://www.silabs.com/
[manual]: https://www.silabs.com/documents/public/reference-manuals/EFM32HG-RM.pdf
[datasheet]: https://www.silabs.com/documents/public/data-sheets/EFM32HG322.pdf

## About the code

This repo is meant as a starting point for developing your own code
for the Bornhack 2019 badge. Like the previous years we're using the
[geckonator][] library, but this year everything is split out
into separate source files. Every file ending in `.c` will
automatically be included in the build.

This way it should be easy to add your own code in a separate file
and add it to the main menu in `main.c`.
Have a look at `buttontest.c`, `showbmp.c` or `dumpir.c` for examples.

For reading the partition table and FAT filesystem on an SD-card we're using
the [FatFs][] library.

[FatFs]: http://elm-chan.org/fsw/ff/00index_e.html
[geckonator]: https://github.com/flummer/geckonator

## Hack your own badge

### 1. Install dependencies

##### Archlinux
```sh
pacman -S arm-none-eabi-gcc arm-none-eabi-newlib make
```

##### Debian/Ubuntu
```sh
apt-get install gcc-arm-none-eabi libnewlib-arm-none-eabi make
```

##### Fedora
```sh
dnf install arm-none-eabi-gcc arm-none-eabi-newlib make
```

##### NixOS/Nix
```sh
nix-shell
```

##### OSX

Download an arm-none-eabi toolchain from ARM [here][arm-toolchain].
Unpack the tarball and update your PATH variable to point to the unpacked `bin` directory.

##### Windows
###### Option 1
Download the apropriate installer from ARM [here][arm-toolchain].
Install it and update your path.
You'll also need GNU Make installed.
A pre-built version can be downloaded [here](http://gnuwin32.sourceforge.net/packages/make.htm).

###### Option 2

Use Windows Subsystem for Linux with Ubuntu 16.04 Xenial or newer and proceed as on Ubuntu above.

### 2. Get the source code

If you already have git installed
```
git clone https://github.com/bornhack/badge2019.git
cd badge2019
git submodule init
git submodule update
```

Otherwise you can download a tarball or zip file from
[https://github.com/bornhack/badge2019](https://github.com/bornhack/badge2019)

### 3. Build the code

Simply type `make` in the downloaded directory.
This should produce a file called `code.bin` in the `out` directory.

### 4. Update your badge

1. Connect a USB cable to the board and your computer.
2. Press the **BOOT** button on the badge (the red LED lights up).
3. Copy `out/code.bin` to the `GECKOBOOT` USB stick that appeared on your computer.
4. Eject (or unmount) the USB stick (the blue LED lights up), the badge reboots and you can watch your code run.

Alternatively if you have [dfu-utils][] installed step 3 and 4 can be replaced by typing
`make dfu` in the source repository.

[dfu-utils]: http://dfu-util.sourceforge.net/
[arm-toolchain]: https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads

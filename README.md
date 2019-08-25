# Bornhack Badge 2019

## About the hardware

The Bornhack 2019 badge is based around a SiLabs Happy Gecko microcontroller,
a 240x240 pixel IPS txt graphical display, a 5 way navigation switch, IR
communications, microSD card, few exra buttons, LEDs and a USB interface.

The badge also have an expansion port following the shitty addon standard
version 1.69bis, with PD6 and PD7 brought out to the two GPIO pins in addition
to I2C.

The microcontroller is a Cortex-M0+ with built in USB, specifically we are using
the EFM32HG322F64G and the display is using the ST7789 controller

The PCB has been designed in KiCad and uses a double sided PCB, with all the
components on the front side (only the battery clips are on the back side).

## About the code

We have separated the hardware design files from the code and put the code in the
[master branch](https://github.com/bornhack/badge2019/tree/master) for you
to have a look at and extend.
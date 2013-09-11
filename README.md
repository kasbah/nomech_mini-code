# How do I program the board I just built? 

## Requirements

Software:
- avr-gcc
- avr-libc
- avrdude

Hardware:
- avrdude compatible ISP programmer

## Instructions

First get the LUFA submodule. 

    git submodules init
    git submodules update

Then edit ```LUFA/LUFA/Build/lufa_avrdude.mk``` to your preference of

    AVRDUDE_PROGRAMMER 
    AVRDUDE_PORT       
    AVRDUDE_FLAGS      

so for example for the AVRISP MkII that is:

    AVRDUDE_PROGRAMMER ?= avrisp2
    AVRDUDE_PORT       ?= usb
    AVRDUDE_FLAGS      ?= -B1

Then you can set the fuses, compile and upload the code.

    cd source
    make first
    make avrdude

## Protocol

The board appears as a virtual serial device (typically ```/dev/ttyACM0``` on Linux) and 
sends and sends the measurement of the first electrode.

    0:<measurement>

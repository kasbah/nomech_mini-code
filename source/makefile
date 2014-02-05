#
#             LUFA Library
#     Copyright (C) Dean Camera, 2013.
#
#  dean [at] fourwalledcubicle [dot] com
#           www.lufa-lib.org
#
# --------------------------------------
#         LUFA Project Makefile.
# --------------------------------------

# Run "make help" for target help.

MCU          = atmega32u4
ARCH         = AVR8
BOARD        = NONE
F_CPU        = 8000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
TARGET       = NoMech
SRC          = $(TARGET).c Descriptors.c LUFA.c $(LUFA_SRC_USB) $(LUFA_SRC_USBCLASS)
LUFA_PATH    = ../LUFA/LUFA
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -IConfig/
LD_FLAGS     =

NOW          = $(shell date --iso=seconds)

# Default target
all:


# Include LUFA build script makefiles
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk
include $(LUFA_PATH)/Build/lufa_doxygen.mk
include $(LUFA_PATH)/Build/lufa_dfu.mk
include $(LUFA_PATH)/Build/lufa_hid.mk
include $(LUFA_PATH)/Build/lufa_avrdude.mk
include $(LUFA_PATH)/Build/lufa_atprogram.mk

# custom targets
first:
	avrdude $(BASE_AVRDUDE_FLAGS) -B 10 -e
	avrdude $(BASE_AVRDUDE_FLAGS) -B 10 -U efuse:w:0xCB:m -U hfuse:w:0xDF:m -U lfuse:w:0xFF:m

backup:
	avrdude $(BASE_AVRDUDE_FLAGS) -U flash:r:backup_$(NOW).hex:i 

readfuse: 
	avrdude $(BASE_AVRDUDE_FLAGS) -U efuse:r:ext.txt:r -U hfuse:r:high.txt:r -U lfuse:r:low.txt:r
	./unpack-fuses

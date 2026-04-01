ifeq ($(and $(strip $(DEVKITPRO)),$(strip $(DEVKITARM))),)
$(error Make sure DEVKITPRO and DEVKITARM are correctly set in your environment.)
endif

# Name of your ROM
#
# Add _mb to the end to build a multiboot ROM.
PROJECT		:= advgm-sync-with-maxmod

# Uncomment this if you're building a library
#
# BUILD_LIB	:= yes

# Options for gbafix (optional)
#
# Title:        12 characters
# Game code:     4 characters
# Maker code:    2 characters
# Version:       1 character
ROM_TITLE	:= ADVGM TEST
ROM_GAMECODE	:= ADVGM
ROM_MAKERCODE	:= CR
ROM_VERSION	:= 0

#
# Files
#
# All options support glob patterns like `src/*.c`.
#

# DMG audio files to process with `tools/advgm_converter.py`
DMG_AUDIO_FILES	:= dmg_audio/*.vgm

# Audio files to process with mmutil
AUDIO_FILES	:= audio/*.it audio/*.xm audio/*.s3m audio/*.mod

# Binary files to process with bin2s
BINARY_FILES	:=

# Graphics files to process with grit
#
# Every file requires an accompanying .grit file,
# so gfx/test.png needs gfx/test.grit
GRAPHICS	:=

# Source files to compile
SOURCES		:= src/*.c src/*.S maxmod/source/core/*.c maxmod/source/gba/*.c maxmod/source/gba/*.s libugba/source/gba/irq_handler.s libugba/source/gba/interrupts_gba.c

# Include directories
INCLUDES	:= src maxmod/include maxmod/source libugba/include

#
# Dependencies
#

# Python executable
PYTHON		:= python

# advgm converter
ADVGM_CONV	:= ../../tools/advgm_converter.py

# Library directories, with /include and /lib
LIBDIRS		:= ../../ $(DEVKITPRO)/libtonc

# Libraries to link
LIBS		:= advgm tonc

#
# Directories
#

# All build output goes here
BUILDDIR	:= build

#
# Build Options
#

# Compiler flags (all languages)
ALLFLAGS	:= -Wall -Wextra -g -gdwarf-4 -O2 \
		-mthumb-interwork \
		-ffunction-sections -fdata-sections \
		-masm-syntax-unified \
		-D_DEFAULT_SOURCE \
		-D__GBA__

# C compiler flags
CFLAGS		:= -std=gnu17

# C++ compiler flags
CXXFLAGS	:= -std=gnu++20 -fno-rtti -fno-exceptions

# Assembler flags (as passed to GCC)
ASFLAGS		:= -x assembler-with-cpp

# Linker flags (as passed to GCC)
LDFLAGS		:= -mthumb \
		$(if $(filter %_mb,$(PROJECT)),-specs=gba_mb.specs,-specs=gba.specs)

# Uncomment this if you want to use Link Time Optimization
#
# USE_LTO		:= yes

include build.mk

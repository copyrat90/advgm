ifeq ($(and $(strip $(DEVKITPRO)),$(strip $(DEVKITARM))),)
$(error Make sure DEVKITPRO and DEVKITARM are correctly set in your environment.)
endif

# Name of your ROM
#
# Add _mb to the end to build a multiboot ROM.
PROJECT		:= advgm-minimal-player

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

# Binary files to process with bin2s
BINARY_FILES	:=

# Graphics files to process with grit
#
# Every file requires an accompanying .grit file,
# so gfx/test.png needs gfx/test.grit
GRAPHICS	:=

# Source files to compile
SOURCES		:= src/*.c

# Include directories
INCLUDES	:= src

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
ALLFLAGS	:= -Wall -Wextra -g3 -gdwarf-4 -O2 \
		-ffunction-sections -fdata-sections \
		-masm-syntax-unified \
		-D_DEFAULT_SOURCE

# C compiler flags
CFLAGS		:= -std=c99

# C++ compiler flags
CXXFLAGS	:= -std=c++20 -fno-rtti -fno-exceptions

# Assembler flags (as passed to GCC)
ASFLAGS		:=

# Linker flags (as passed to GCC)
LDFLAGS		:= -mthumb \
		$(if $(filter %_mb,$(PROJECT)),-specs=gba_mb.specs,-specs=gba.specs)

# Uncomment this if you want to use Link Time Optimization
#
# USE_LTO		:= yes

include build.mk

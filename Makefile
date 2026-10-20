
TARGET := coastal-racer
BUILD := build
SOURCES := source
DATA :=
INCLUDES :=
GRAPHICS := gfx
GFXBUILD := $(BUILD)

ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS := -g -Wall -O2 -mword-relocations \
          -ffunction-sections $(ARCH) -D__3DS__

CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11
ASFLAGS := -g $(ARCH)
LDFLAGS = -specs=3dsx.specs -g $(ARCH)

LIBS := -lcitro3d -lctru -lm
LIBDIRS := $(CTRULIB)

include $(DEVKITARM)/3ds_rules

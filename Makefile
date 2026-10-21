
# Coastal Racer 3DS
# devkitPro / devkitARM / libctru / Citro3D / Citro2D

.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error DEVKITARM is not set. Use the devkitPro devkitARM environment.)
endif

TOPDIR ?= $(CURDIR)

include $(DEVKITARM)/3ds_rules

TARGET      := coastal-racer-3ds
BUILD       := build
SOURCES     := source
INCLUDES    := source build
DATA        := assets

ARCH        := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS      = -g -Wall -Wextra -O2 -mword-relocations \
              -ffunction-sections -fdata-sections $(ARCH) \
              $(INCLUDE) -D__3DS__

CXXFLAGS    = $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11
ASFLAGS     := $(ARCH)

LDFLAGS     = -specs=3dsx.specs -g $(ARCH) \
              -Wl,-Map,$(notdir $*.map)

LIBS        := -lcitro2d -lcitro3d -lctru -lm
LIBDIRS     := $(CTRULIB)

ifneq ($(BUILD),$(notdir $(CURDIR)))

# Корневая папка проекта
export TOPDIR  := $(CURDIR)
export OUTPUT  := $(CURDIR)/$(TARGET)
export VPATH   := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

# Исходники
CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

ifeq ($(strip $(CPPFILES)),)
    export LD := $(CC)
else
    export LD := $(CXX)
endif

# Объектные файлы
export OFILES := $(CPPFILES:.cpp=.o) \
                 $(CFILES:.c=.o) \
                 $(SFILES:.s=.o) \
                 vshader.v.pica.o

# Пути поиска заголовков
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include)

export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: all clean

all: $(BUILD)

$(BUILD):
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(TOPDIR)/Makefile

clean:
	@echo "Cleaning Coastal Racer 3DS..."
	@rm -rf $(BUILD) $(TARGET).3dsx $(TARGET).elf $(TARGET).smdh

else

DEPENDS := $(OFILES:.o=.d)

.PHONY: all

all: $(OUTPUT).3dsx

$(OUTPUT).3dsx: $(OUTPUT).elf

$(OUTPUT).elf: $(OFILES)

# Сначала создаём шейдер и заголовок,
# затем компилируем main.c
main.o: vshader_shbin.h

# Корректный абсолютный путь к исходному шейдеру.
# Рецепт создаёт и объектный файл, и заголовок.
vshader.v.pica.o: $(TOPDIR)/source/vshader.v.pica
	@echo "Compiling PICA200 vertex shader..."
	@picasso -o vshader.shbin $<
	@bin2s -a 4 vshader.shbin > vshader.s
	@$(AS) -o vshader.v.pica.o vshader.s
	@echo "extern const u8 vshader_shbin[];" > vshader_shbin.h
	@echo "extern const u8 vshader_shbin_end[];" >> vshader_shbin.h
	@echo "extern const u32 vshader_shbin_size;" >> vshader_shbin.h
	@rm -f vshader.shbin vshader.s

# Заголовок создаётся рецептом выше
vshader_shbin.h: vshader.v.pica.o

-include $(DEPENDS)

endif


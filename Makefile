#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET		:=	coastal-racer-3ds
BUILD		:=	build
SOURCES		:=	source
INCLUDES	:=	source

ARCH		:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS		:=	-g -Wall -O2 -mword-relocations -ffunction-sections -fdata-sections $(ARCH) $(INCLUDE) -D__3DS__
CXXFLAGS	:=	$(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11
ASFLAGS		:=	$(ARCH)
LDFLAGS		=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS		:=	-lcitro2d -lcitro3d -lctru -lm
LIBDIRS		:=	$(CTRULIB)

ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------
export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

# Явно добавляем объектный файл шейдера в список сборки
export OFILES	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o) vshader.v.pica.o

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean all
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(OUTPUT).smdh $(TARGET).elf

#---------------------------------------------------------------------------------
else
.PHONY:	all
DEPENDS	:=	$(OFILES:.o=.d)

all	:	$(OUTPUT).3dsx
$(OUTPUT).3dsx	:	$(OUTPUT).elf
$(OUTPUT).elf	:	$(OFILES)

# Заставляем main.o ждать заголовочный файл
main.o: vshader_shbin.h

# ЖЕСТКОЕ ПРАВИЛО: берем vshader.v.pica и делаем из него и .o, и .h
vshader.v.pica.o vshader_shbin.h: $(CURDIR)/source/vshader.v.pica
	@echo "Compiling shader..."
	@picasso -o vshader.shbin $<
	@bin2s -a 4 vshader.shbin > vshader.s
	@$(AS) -o vshader.v.pica.o vshader.s
	@echo "extern const u8 vshader_shbin[];" > vshader_shbin.h
	@echo "extern const u8 vshader_shbin_end[];" >> vshader_shbin.h
	@echo "extern const u32 vshader_shbin_size;" >> vshader_shbin.h
	@rm vshader.shbin vshader.s

-include $(DEPENDS)
#---------------------------------------------------------------------------------------
endif
#-----------------------------------------------------------------------------------------

TARGET := python_embed
BUILD := build
SOURCES := source
DATA := data
ROMFS := romfs

ifeq ($(strip $(DEVKITPRO)),)
$(error DEVKITPRO is not set)
endif

include $(DEVKITPRO)/libnx/switch_rules

PYTHON_PC ?= python3-embed
PYTHON_CFLAGS := $(shell PKG_CONFIG_LIBDIR=$(DEVKITPRO)/portlibs/switch/lib/pkgconfig pkg-config --cflags $(PYTHON_PC))
PYTHON_LIBS := $(shell PKG_CONFIG_LIBDIR=$(DEVKITPRO)/portlibs/switch/lib/pkgconfig pkg-config --libs $(PYTHON_PC))

CFLAGS += -g -Wall -O2 -ffunction-sections $(PYTHON_CFLAGS)
CFLAGS += $(ARCH) -D__SWITCH__
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS := -g $(ARCH)
LDFLAGS = -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS := $(PYTHON_LIBS) -lnx
LIBDIRS := $(PORTLIBS) $(LIBNX)

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

export OFILES_BIN := $(addsuffix .o,$(BINFILES))
export OFILES_SRC := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES := $(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN := $(addsuffix .h,$(subst .,_,$(BINFILES)))

.PHONY: all clean

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@rm -rf $(BUILD) $(TARGET).elf $(TARGET).nro $(TARGET).nacp

else

DEPENDS := $(OFILES:.o=.d)

.PHONY: all

all: $(OUTPUT).nro

$(OUTPUT).nro: $(OUTPUT).elf

$(OUTPUT).elf: $(OFILES)

%.bin.o %.bin.h: %.bin
	@$(bin2o)

-include $(DEPENDS)

endif

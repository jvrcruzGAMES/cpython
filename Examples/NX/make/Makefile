TARGET := python_switch_features
BUILD := build
SOURCES := source
DATA := data
INCLUDES := include
ROMFS := romfs

APP_TITLE := Python Switch Features
APP_AUTHOR := CPython Switch Fork
APP_VERSION := 1.0.0
APP_ICON ?= $(DEVKITPRO)/libnx/default_icon.jpg

ifeq ($(strip $(DEVKITPRO)),)
$(error DEVKITPRO is not set)
endif

include $(DEVKITPRO)/libnx/switch_rules

PYTHON_PREFIX ?= $(DEVKITPRO)/portlibs/switch
PYTHON_PC ?= python-3.14-embed
PYTHON_PKG_CONFIG := PKG_CONFIG_LIBDIR=$(PYTHON_PREFIX)/lib/pkgconfig pkg-config --define-variable=prefix=$(PYTHON_PREFIX) --define-variable=exec_prefix=$(PYTHON_PREFIX)
PYTHON_CFLAGS := $(shell $(PYTHON_PKG_CONFIG) --cflags $(PYTHON_PC))
PYTHON_LIBS := $(shell $(PYTHON_PKG_CONFIG) --static --libs $(PYTHON_PC))

CFLAGS += -g -Wall -O2 -ffunction-sections $(PYTHON_CFLAGS)
CFLAGS += $(ARCH) $(INCLUDE) -D__SWITCH__
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS := -g $(ARCH)
LDFLAGS = -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map) -Wl,-z,notext -Wl,-u,__stacksize__
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

ifeq ($(strip $(CPPFILES)),)
export LD := $(CC)
else
export LD := $(CXX)
endif

export OFILES_BIN := $(addsuffix .o,$(BINFILES))
export OFILES_SRC := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES := $(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN := $(addsuffix .h,$(subst .,_,$(BINFILES)))
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: all clean prepare-romfs

all: $(BUILD)

prepare-romfs:
	@test -d "$(PYTHON_PREFIX)/share/python-switch/romfs/python" || { echo "Missing $(PYTHON_PREFIX)/share/python-switch/romfs/python"; exit 1; }
	@! find "$(PYTHON_PREFIX)/share/python-switch/romfs/python" -type f \( -name '*.so' -o -name '*.pyd' -o -name '*.nro' -o -name '*.nso' -o -name '*.elf' \) -print | grep . || { echo "Native Python payloads cannot be packed into romfs; link them or package them in an executable title layout."; exit 1; }
	@rm -rf romfs/python
	@mkdir -p romfs
	@cp -R "$(PYTHON_PREFIX)/share/python-switch/romfs/python" romfs/

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@rm -rf $(BUILD) $(TARGET).elf $(TARGET).nro $(TARGET).nacp

else

DEPENDS := $(OFILES:.o=.d)

.PHONY: all

all: $(OUTPUT).nro

$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp
	@elf2nro $< $@ --icon=$(APP_ICON) --nacp=$(OUTPUT).nacp --romfsdir=$(TOPDIR)/$(ROMFS) $(NROFLAGS)
	@echo built ... $(notdir $@)

$(OUTPUT).elf: $(OFILES)

%.bin.o %.bin.h: %.bin
	@$(bin2o)

-include $(DEPENDS)

endif

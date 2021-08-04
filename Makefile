# Global definitions
platform   ?= unix

CC        = $(CROSS_COMPILE)gcc
SYSROOT   = $(shell $(CC) --print-sysroot)

OBJS      = libpicofe/input.o libpicofe/in_sdl.o libpicofe/linux/in_evdev.o libpicofe/linux/plat.o libpicofe/fonts.o libpicofe/readpng.o libpicofe/config_file.o config.o core.o menu.o main.o options.o scale.o unzip.o

BIN       = picoarch

CFLAGS     += -Wall
CFLAGS     += -fdata-sections -ffunction-sections -DPICO_HOME_DIR='"/.picoarch/"' -flto -fno-PIC
CFLAGS     += -I./ $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)

LDFLAGS    = -lc -ldl -lgcc -lSDL -lasound -lpng -lz -Wl,--gc-sections -flto -fno-PIC

CORES      = gpsp snes9x2002 snes9x2005

ifeq ($(platform), trimui)
	OBJS += plat_trimui.o
	CFLAGS += -mcpu=arm926ej-s -mtune=arm926ej-s

else ifeq ($(platform), unix)
	OBJS += plat_linux.o
	LDFLAGS += -fPIE
endif

ifeq ($(DEBUG), 1)
	CFLAGS += -Og -g
else
	CFLAGS += -Ofast -DNDEBUG

ifneq ($(PROFILE), 1)
	LDFLAGS += -s
endif

endif

ifeq ($(PROFILE), 1)
	CFLAGS += -fno-omit-frame-pointer -pg -g
	LDFLAGS += -pg -g
else ifeq ($(PROFILE), GENERATE)
	CFLAGS	+= -fprofile-generate=./profile/picoarch
	LDFLAGS	+= -lgcov
else ifeq ($(PROFILE), APPLY)
	CFLAGS	+= -fprofile-use -fprofile-dir=./profile/picoarch -fbranch-probabilities
endif

ifeq ($(MINUI), 1)
	MMENU = 1
	CFLAGS += -DMINUI
endif

ifeq ($(MMENU), 1)
	CFLAGS += -DMMENU
	LDFLAGS += -lSDL_image -lSDL_ttf -ldl
endif

SOFILES = $(foreach core,$(CORES),$(core)_libretro.so)

all: $(BIN) cores

plat_trimui.o: plat_sdl.c
plat_linux.o: plat_sdl.c

$(BIN): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(BIN)

define CORE_template =

$1_REPO ?= https://github.com/libretro/$(1)/

$1_MAKE = make $(and $($1_MAKEFILE),-f $($1_MAKEFILE)) platform=$(platform) $(and $(DEBUG),DEBUG=$(DEBUG)) $(and $(PROFILE),PROFILE=$(PROFILE)) $($(1)_FLAGS)

$(1):
	git clone $(if $($1_REVISION),,--depth 1) $$($(1)_REPO) $(1)
	$(if $1_REVISION,cd $(1) && git checkout $($1_REVISION),)
	(test ! -d patches/$(1)) || (cd $(1) && $(foreach patch, $(sort $(wildcard patches/$(1)/*.patch)), patch -p1 < ../$(patch) &&) true)

$(1)_libretro.so: $(1)
	cd $(1) && $$($1_MAKE)
	cp $(1)/$(1)_libretro.so .

clean-$(1):
	test ! -d $(1) || cd $(1) && $$($1_MAKE) clean
	rm -f $(1)_libretro.so
endef

$(foreach core,$(CORES),$(eval $(call CORE_template,$(core))))

cores: $(SOFILES)

clean:
	rm -f $(OBJS) $(BIN) $(SOFILES)
	rm -rf pkg

clean-all: $(foreach core,$(CORES),clean-$(core)) clean

force-clean: clean
	rm -rf $(CORES)

ifeq ($(platform), trimui)

gpsp_ROM_DIR = GBA
gpsp_TYPES = gba,bin,zip
gpsp_PAK_NAME = Game Boy Advance
define gpsp_PAK_EXTRA

needs-swap

endef

snes9x2002_ROM_DIR = SFC
snes9x2002_TYPES = smc,fig,sfc,gd3,gd7,dx2,bsx,swc
snes9x2002_PAK_NAME = Super Nintendo

snes9x2005_ROM_DIR = SFC
snes9x2005_TYPES = smc,fig,sfc,gd3,gd7,dx2,bsx,swc
snes9x2005_PAK_NAME = Super Nintendo (2005)

# -- gmenunx

dist-gmenu-section:
	mkdir -p pkg/gmenunx/Apps/picoarch
	mkdir -p pkg/gmenunx/Apps/gmenunx/sections/libretro
	touch pkg/gmenunx/Apps/gmenunx/sections/libretro/.section

define CORE_gmenushortcut =

define $1_SHORTCUT
title=$1
exec=/mnt/SDCARD/Apps/picoarch/picoarch
params=/mnt/SDCARD/Apps/picoarch/$1_libretro.so
selectordir=/mnt/SDCARD/Roms/$($1_ROM_DIR)
selectorfilter=$($1_TYPES)
endef

dist-gmenu-$(1): $(BIN) $(1)_libretro.so dist-gmenu-section
	cp $(BIN) $1_libretro.so pkg/gmenunx/Apps/picoarch
	$$(file >pkg/gmenunx/Apps/gmenunx/sections/libretro/$(1),$$($(1)_SHORTCUT))

endef

$(foreach core, $(CORES),$(eval $(call CORE_gmenushortcut,$(core))))

dist-gmenu: $(foreach core, $(CORES), dist-gmenu-$(core))

# -- MinUI

ifeq ($(MINUI), 1)
define CORE_pak_template =

define $1_LAUNCH_SH
#!/bin/sh
# $($1_PAK_NAME).pak/launch.sh

EMU_EXE=picoarch
EMU_DIR=$$$$(dirname "$$$$0")
ROM_DIR=$$$${EMU_DIR/.pak/}
ROM_DIR=$$$${ROM_DIR/Emus/Roms}
EMU_NAME=$$$${ROM_DIR/\/mnt\/SDCARD\/Roms\//}
ROM=$$$${1}
$($1_PAK_EXTRA)
HOME="$$$$ROM_DIR"
cd "$$$$EMU_DIR"
"$$$$EMU_DIR/$$$$EMU_EXE" ./$1_libretro.so "$$$$ROM" &> "/mnt/SDCARD/.minui/logs/$$$$EMU_NAME.txt"
endef

dist-minui-$(1): $(BIN) $(1)_libretro.so
	mkdir -p "pkg/MinUI/Emus/$($1_PAK_NAME).pak"
	$$(file >$1_launch.sh,$$($1_LAUNCH_SH))
	mv $1_launch.sh "pkg/MinUI/Emus/$($1_PAK_NAME).pak/launch.sh"
	cp $(BIN) $1_libretro.so "pkg/MinUI/Emus/$($1_PAK_NAME).pak"

endef

$(foreach core, $(CORES),$(eval $(call CORE_pak_template,$(core))))

dist-minui: $(foreach core, $(CORES), dist-minui-$(core))

endif # MINUI=1

endif # platform=trimui

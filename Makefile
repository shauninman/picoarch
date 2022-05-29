# Global definitions
platform   ?= unix

CC        = $(CROSS_COMPILE)gcc
SYSROOT   = $(shell $(CC) --print-sysroot)

PROCS     = -j4

OBJS      = libpicofe/input.o libpicofe/in_sdl.o libpicofe/linux/in_evdev.o \
            libpicofe/linux/plat.o libpicofe/fonts.o libpicofe/readpng.o \
			libpicofe/config_file.o source/cheat.o source/config.o \
			source/content.o source/core.o source/menu.o source/main.o \
			source/options.o source/overrides.o source/patch.o source/scale.o \
			source/scaler_neon.o source/unzip.o source/util.o

BIN       = picoarch

CFLAGS     += -Wall
CFLAGS     += -fdata-sections -ffunction-sections -DPICO_HOME_DIR='"/.picoarch/"' -flto
CFLAGS     += -I./ -I./libretro-common/include/ -I./source $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)

LDFLAGS    = -lc -ldl -lgcc -lm -lSDL -lpng12 -lz -Wl,--gc-sections -flto

PATCH = git apply

# Unpolished or slow cores that build
# EXTRA_CORES += mame2003_plus
CORES = gambatte gpsp pokemini fceumm snes9x2005_plus pcsx_rearmed picodrive mgba smsplus-gx beetle-pce-fast genesis-plus-gx snes9x2005 nxengine supafaust

nxengine_REPO = https://github.com/libretro/nxengine-libretro

supafaust_CORE = mednafen_supafaust_libretro.so

snes9x2005_plus_REPO = https://github.com/libretro/snes9x2005
snes9x2005_plus_FLAGS = USE_BLARGG_APU=1

beetle-pce-fast_REPO = https://github.com/libretro/beetle-pce-fast-libretro
beetle-pce-fast_CORE = mednafen_pce_fast_libretro.so

beetle-vb_REPO = https://github.com/libretro/beetle-vb-libretro
beetle-vb_CORE = mednafen_vb_libretro.so

bluemsx_REPO = https://github.com/libretro/blueMSX-libretro

fbalpha2012_BUILD_PATH = fbalpha2012/svn-current/trunk
fbalpha2012_MAKEFILE = makefile.libretro

fceumm_REPO = https://github.com/libretro/libretro-fceumm
fceumm_MAKEFILE = Makefile.libretro

fmsx_REPO = https://github.com/libretro/fmsx-libretro

gambatte_REPO = https://github.com/libretro/gambatte-libretro

snes9x_REPO = https://github.com/libretro/snes9x

gme_REPO = https://github.com/libretro/libretro-gme

mame2000_REPO = https://github.com/libretro/mame2000-libretro
mame2003_plus_REPO = https://github.com/libretro/mame2003-plus-libretro

pcsx_rearmed_MAKEFILE = Makefile.libretro
# the next commit breaks trimui patches
pcsx_rearmed_REVISION = 131a1b41835bc0eba3e35998dce376163a0a1b14 

picodrive_REPO = https://github.com/irixxxx/picodrive
picodrive_MAKEFILE = Makefile.libretro

pokemini_REPO = https://github.com/libretro/PokeMini
pokemini_MAKEFILE = Makefile.libretro

quicknes_REPO = https://github.com/libretro/QuickNES_Core

smsplus-gx_MAKEFILE = Makefile.libretro
smsplus-gx_CORE = smsplus_libretro.so

genesis-plus-gx_MAKEFILE = Makefile.libretro
genesis-plus-gx_CORE = genesis_plus_gx_libretro.so

ifeq ($(platform), trimui)
	OBJS += source/plat_trimui.o
	CFLAGS += -mcpu=arm926ej-s -mtune=arm926ej-s -fno-PIC -DCONTENT_DIR='"/mnt/SDCARD/Roms"'
	LDFLAGS += -fno-PIC
else ifeq ($(platform), miyoomini)
	OBJS += source/plat_miyoomini.o
	CFLAGS += -marm -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -march=armv7ve -fPIC -DCONTENT_DIR='"/mnt/SDCARD/Roms"'
	LDFLAGS += -fPIC -lmi_sys -lmi_gfx
	MMENU=1
	# PATCH=patch
else ifeq ($(platform), unix)
	OBJS += source/plat_linux.o
	LDFLAGS += -fPIE
endif

ifeq ($(DEBUG), 1)
	CFLAGS += -Og -g
	LDFLAGS += -g
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

CFLAGS += $(EXTRA_CFLAGS)

SOFILES = $(foreach core,$(CORES),$(core)_libretro.so)

print-%:
	@echo '$*=$($*)'

all: $(BIN) cores

libpicofe/.patched:
	cd libpicofe && $(PATCH) -p1 < ../patches/libpicofe/0001-key-combos.patch && touch .patched

clean-libpicofe:
	test ! -f libpicofe/.patched || (cd libpicofe && $(PATCH) -p1 -R < ../patches/libpicofe/0001-key-combos.patch && rm .patched)

plat_miyoomini.o: plat_sdl.c
plat_trimui.o: plat_sdl.c
plat_linux.o: plat_sdl.c

$(BIN): libpicofe/.patched $(OBJS)
	mkdir -p output
	$(CC) $(OBJS) $(LDFLAGS) -o output/$(BIN)

define CORE_template =

$1_REPO ?= https://github.com/libretro/$(1)/

$1_BUILD_PATH ?= cores/$(1)

$1_MAKE = make $(and $($1_MAKEFILE),-f $($1_MAKEFILE)) platform=$(platform) $(and $(DEBUG),DEBUG=$(DEBUG)) $(and $(PROFILE),PROFILE=$(PROFILE)) $($(1)_FLAGS)

clone-$(1):
	mkdir -p cores
	cd cores && git clone $(if $($1_REVISION),,--depth 1) --recursive $$($(1)_REPO) $(1)
	$(if $1_REVISION,cd $$($1_BUILD_PATH) && git checkout $($1_REVISION),)

cores/$(1):
	mkdir -p cores
	cd cores && git clone $(if $($1_REVISION),,--depth 1) --recursive $$($(1)_REPO) $(1)
	$(if $1_REVISION,cd $$($1_BUILD_PATH) && git checkout $($1_REVISION),)
	(test ! -d patches/$(1)) || (cd $$($1_BUILD_PATH) && $(foreach patch, $(sort $(wildcard patches/$(1)/*.patch)), $(PATCH) -p1 < ../../$(patch) &&) true)

$(1): cores/$(1)

$(1)_libretro.so: $(1)
	mkdir -p output
	cd $$($1_BUILD_PATH) && $$($1_MAKE) $(PROCS)
	cp $$($1_BUILD_PATH)/$(if $($(1)_CORE),$($(1)_CORE),$(1)_libretro.so) output/$(1)_libretro.so

clean-$(1):
	test ! -d $(1) || cd $$($1_BUILD_PATH) && $$($1_MAKE) clean
	rm -rf $(1)_libretro.so
endef

$(foreach core,$(CORES),$(eval $(call CORE_template,$(core))))

cores: $(SOFILES)

clean: # clean-libpicofe
	rm -f $(OBJS) $(SOFILES)
	rm -rf output
	rm -rf pkg

clean-all: $(foreach core,$(CORES),clean-$(core)) clean

force-clean: clean
	rm -rf $(CORES)

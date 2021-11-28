# picoarch - a libretro frontend designed for small screens and low power

picoarch runs libretro cores (emulators) for various systems with low overhead and UI designed for small screen, low-powered devices like the Trimui Model S (PowKiddy A66).

It supports: 

- **Arcade** (mame2000)
- **Colecovision** (blueMSX, smsplus)
- **Game Boy / Game Boy Color** (gambatte)
- **Game Boy Advance** (gpsp)
- **Game Gear** (picodrive, smsplus)
- **Genesis** (picodrive)
- **MSX** (fMSX, blueMSX)
- **NES** (quicknes, fceumm)
- **Sega Master System** (picodrive, smsplus)
- **Super NES** (snes9x2002, snes9x2005)
- **PCE / TurboGrafx-16** (beetle-pce-fast)
- **PlayStation** (pcsx_rearmed)
- more to come

picoarch can also play game music (gme).

All emulators have:

- fast-forward
- soft scaling options
- menu+button combo keybindings
- per-game config
- screenshots

Most have:
- cheat support
- IPS/BPS softpatching
- auto-frameskip for smooth audio

## Install

### gmenunx

Extract the contents of the `gmenunx/` directory onto the SD card. You will have a `libretro` section with launchers for all of the emulators. You will also have `picoarch` in the emulators section where you can run any core in `/Apps/picoarch/`.

### MinUI

If you want to replace built-in .paks, simply extract `MinUI/` onto the SD card. 

If you want to replace individual emulators, overwrite the existing MinUI .pak with the picoarch version.

If you want to use picoarch as a single application instead of replacing .paks, copy `MinUI/Games` onto the SD card.

## Notes / extra features

### Bios

Some emulators require bios files. For gmenunx, bios files are placed into `/Apps/picoarch/system`. For MinUI, they are placed in the save directory, similar to other .paks.

The libretro documentation specifies which bios is required for each core. For example, needed fMSX bios files are listed here: <https://docs.libretro.com/library/fmsx/>

### Cheats

Cheats use RetroArch .cht file format. Many cheat files are here <https://github.com/libretro/libretro-database/tree/master/cht>

Cheat file name needs to match ROM name, and go underneath save directory. For example, `/Apps/.picoarch-gambatte/cheats/Super Mario Land (World).cht`. When a cheat file is detected, a "cheats" menu item will appear. Not all cheats work with all cores, may want to clean up files to just the cheats you want.

### IPS / BPS soft-patching

Many cores can apply patches when loading. For example, loading `/roms/game.gba` will apply patches named `/roms/game.ips`, `/roms/game.ips1`, `/roms/game.IPS2`, `/roms/game.bps`, etc. Patching is temporary, original files are unmodified. Patches are loaded in case-insensitive alphabetical order. Note that `.ips12` loads before `.ips2`, but after `.ips02`.

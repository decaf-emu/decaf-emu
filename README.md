[![Build status](https://ci.appveyor.com/api/projects/status/fbhhy0nf6nym9pcf?svg=true)](https://ci.appveyor.com/project/exjam/decaf-emu) [![Build Status](https://travis-ci.org/decaf-emu/decaf-emu.svg?branch=master)](https://travis-ci.org/decaf-emu/decaf-emu)

# decaf-emu
Researching Wii U emulation. Licensed under the terms of the GNU General Public License, version 2 or later (GPLv2+).

You can find us at #wiiu-emu on freenode.

## Requirements
- Windows with Visual Studio 2015 Update 3
- Linux with a modern C++14 friendly compiler (gcc, clang)
- 64 bit
- OpenGL 4.5 (using direct state access, available on older OpenGL versions as an extension).

## Compatibility
- None

## Building

There are two decaf targets:
- decaf-sdl - Default emulator target using SDL for window creation and input.
- decaf-cli - Command line only which will run games with no graphics or inputs, useful for test .rpx files.

### Windows with VS2015 Update 3
- Open decaf.sln
- `ReleaseDebug` is recommended for development, `Debug` runs too slow and `Release` takes a long to compile due to LTCG and does not have full debug info.

### Linux
- The cmake builds will depend on your system's zlib, sdl2 - rather than using submodules for everything like on Windows.
- Requires a modern gcc or clang which supports C++11/14 features
- `cmake ../decaf-emu && make`

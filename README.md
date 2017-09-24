[![Build status](https://ci.appveyor.com/api/projects/status/fbhhy0nf6nym9pcf/branch/master?svg=true)](https://ci.appveyor.com/project/exjam/decaf-emu) [![Build Status](https://travis-ci.org/decaf-emu/decaf-emu.svg?branch=master)](https://travis-ci.org/decaf-emu/decaf-emu)

# decaf-emu
Researching Wii U emulation. Licensed under the terms of the GNU General Public License, version 3 or later (GPLv3+).

You can find us for developer discussion at #wiiu-emu on freenode.

## Requirements
- Windows with Visual Studio 2017 Update 3
- Linux with a modern C++17 friendly compiler
- 64 bit
- OpenGL 4.5
- CMake v3.2+

## Compatibility
- None

## Support
- None, this is an in-development project and user support is not provided.

## Binaries
The latest Windows AppVeyor build is available from:
- https://ci.appveyor.com/project/exjam/decaf-emu/build/artifacts

## Building from Source

This project makes use of submodules, please ensure you have cloned them properly using:
- `git submodule update --init`

There are two decaf frontends:
- decaf-sdl - Default emulator target using SDL for window creation and input.
- decaf-cli - Command line only which will run games with no graphics or inputs, useful for test .rpx files.

### Windows with VS2017
- Use CMake to generate a solution

### Linux
- `cmake ../decaf-emu && make`
- It is suggested (but optional) to use your system's zlib, sdl2 and glbinding
- You can enable building with valgrind with -DDECAF_VALGRIND=ON, this requires valgrind to be installed on your system
- Requires a modern gcc or clang which supports C++14 features

## Running

`./decaf-sdl play <path to game>`

It is recommended to run the emulator from the root git directory so that it is able to access `resources/fonts/*`.  Alternatively, set `resources_path` in the configuration file to point to the resources directory.

Configuration files can be found at:
- Windows - `%APPDATA%\decaf`
- Linux - `~/.config/decaf`

On Linux, a "Bus error" crash usually indicates an out-of-space error in the temporary directory.  Set the `TMPDIR` environment variable to a directory on a filesystem with at least 2GB free.

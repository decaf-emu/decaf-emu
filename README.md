[![Build status](https://ci.appveyor.com/api/projects/status/fbhhy0nf6nym9pcf?svg=true)](https://ci.appveyor.com/project/exjam/decaf-emu) [![Build Status](https://travis-ci.org/decaf-emu/decaf-emu.svg?branch=master)](https://travis-ci.org/decaf-emu/decaf-emu)

# decaf-emu
Researching Wii U emulation. Licensed under the terms of the GNU General Public License, version 3 or later (GPLv3+).

You can find us for developer discussion at #wiiu-emu on freenode.

## Requirements
- Windows with Visual Studio 2015 Update 3
- Linux with a modern C++14 friendly compiler
- 64 bit
- OpenGL 4.5
- CMake v3.2+

## Compatibility
- None

## Support
- None, this is an in-development project and user support is not provided.

## Binaries
The latest Windows AppVeyor build is available from:
- https://ci.appveyor.com/api/projects/exjam/decaf-emu/artifacts/decaf-bin.zip?branch=master

## Building from Source

This project makes use of submodules, please ensure you have cloned them properly using:
- `git submodule update --init`

The libbinrec submodule is a Mercurial repository, so install git-remote-hg from https://github.com/felipec/git-remote-hg (and set GIT_ALLOW_PROTOCOL=hg in your environment) or clone it manually.

There are two decaf targets:
- decaf-sdl - Default emulator target using SDL for window creation and input.
- decaf-cli - Command line only which will run games with no graphics or inputs, useful for test .rpx files.

### Windows with VS2015 Update 3
- Use CMake to generate a solution

### Linux
- `cmake ../decaf-emu && make`
- It is suggested (but optional) to use your system's zlib, sdl2 and glbinding
- You can enable building with valgrind with -DDECAF_VALGRIND=ON, this requires valgrind to be installed on your system
- Requires a modern gcc or clang which supports C++11/14 features

## Running

`./decaf-sdl play <path to game>`

It is recommend to run the emulator from the root git directory so that it is able to access `resources/fonts/*`

Configuration files can be found at:
- Windows - `%APPDATA%\decaf`
- Linux - `~/.config/decaf`

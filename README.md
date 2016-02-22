[![Build status](https://ci.appveyor.com/api/projects/status/fbhhy0nf6nym9pcf?svg=true)](https://ci.appveyor.com/project/exjam/decaf-emu) [![Build Status](https://travis-ci.org/decaf-emu/decaf-emu.svg?branch=master)](https://travis-ci.org/decaf-emu/decaf-emu)

# decaf-emu
Researching Wii U emulation. Licensed under the terms of the GNU General Public License, version 2 or later (GPLv2+).

## Requirements
- Windows with Visual Studio 2015
- Linux with a modern C++11/14 friendly compiler (gcc, clang)
- OS X with homebrew clang, not the one included with Xcode
- 64 bit
- OpenGL 4.5 (using direct state access, available on older OpenGL versions as an extension).

## Compatibility
- None

## Building

### Windows with VS2015
- Open decaf.sln
- `ReleaseDebug` is recommended for development, `Debug` runs too slow and `Release` takes too long to compile

### cmake (for Linux and OS X)
- The cmake builds will depend on your system's zlib, sdl2 and glfw - rather than using submodules like on Windows.
- You need to compile with one or both of GLFW or SDL, you can toggle which ones are enabled by passing flags to cmake:
- `-DDECAF_GLFW={OFF,ON}` disable / enable compiling with GLFW, enabled by default
- `-DDECAF_SDL={OFF,ON}` disable / enable compiling with SDL, enabled by default

### Linux
- Requires a modern gcc or clang which supports C++11/14 features
- `cmake ../decaf-emu && make`

### OS X
- Note we are currently using OpenGL features that OS X does not support, currently this is not a supported platform but it does compile at least.
- Requires clang from homebrew `brew install llvm --with-clang --with-libcxx --with-lld`
- Point cmake towards your brew installed clang compiler `cmake ../decaf-emu -DCMAKE_C_COMPILER=/usr/local/opt/llvm/bin/clang -DCMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++`

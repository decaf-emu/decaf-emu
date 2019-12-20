# Building decaf-emu from source
- [Windows](#windows)
- [Linux](#Linux)
- [MacOS](#MacOS)
- [CMake](#CMake)
- [Troubleshooting](#Troubleshooting)

## Windows

### Dependencies
Required:
- [Visual Studio 2019](https://visualstudio.microsoft.com/vs/community/)
- [CMake](https://cmake.org/)
- [Conan](https://conan.io/downloads.html)
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#windows)

By default conan will use `%USERPROFILE%/.conan`, if you have limited C:/ drive space it is recommend to set the `CONAN_USER_HOME` environment variable to a folder on a drive with more space.

### Building
- `git clone --recursive https://github.com/decaf-emu/decaf-emu.git`
- `cd decaf-emu`
- `mkdir build`
- `cd build`
- `conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan`
- `conan install .. -o silent=True`
- Use cmake-gui to generate a VS project file

## Linux

### Dependencies
Required:
- A modern C++17 friendly compiler such as g++8
- CMake
- c-ares
- libcurl
- libuv
- openssl
- sdl2
- zlib

Optional:
- ffmpeg, disable by using `-DDECAF_FFMPEG=OFF`
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#windows), disable by using `-DDECAF_VULKAN=OFF`
- Qt, disable by using `-DDECAF_QT=OFF`

The dependencies, other than Vulkan, can either be acquired using your package manager such as:
- Required: `apt install cmake libcurl4-openssl-dev libsdl2-dev libssl-dev zlib1g-dev`
- Optional: `apt install libavcodec-dev libavfilter-dev libavutil-dev libswscale-dev qtbase5-dev qtbase5-private-dev libqt5svg5-dev`

Or by using [Conan](https://conan.io), which is recommended to be installed using Python pip:
- `pip install conan`
- `conan user`
- `conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan`

It is recommended to use your system's openssl and curl even when using conan:
- `conan install .. --build=missing -o curl=False -o openssl=False`

You may choose to use a mix of system dependenices such as if your system's ffmpeg is too old, you can interactively customise which dependencies to acquire when using `conan install ..` or you can specifiy on the command line. So for example to acquire only ffmpeg you might use `conan install .. --build=missing -o silent=True -o ffmpeg=True -o curl=False -o openssl=False -o sdl2=False -o zlib=False`

### Building
- `git clone --recursive https://github.com/decaf-emu/decaf-emu.git`
- `cd decaf-emu`
- `mkdir build`
- `cd build`
- If using conan: `conan install .. -o silent=True -o curl=False -o openssl=False`
- `cmake -DCMAKE_BUILD_TYPE=Release ../`
- `make`

## MacOS
Currently decaf-emu can build on MacOS using Xcode 11 although MoltenVK is missing crucial features which will prevent most games from rendering correctly, e.g. geometry shaders, transform feedback, logic op support, unrestricted depth range. This means the platform should be considered as unsupported.

I have had success at building with:
- Install [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#mac)
- `brew install openssl sdl2 conan`
- `git clone --recursive https://github.com/decaf-emu/decaf-emu.git`
- `cd decaf-emu`
- `mkdir build && cd build`
- `conan install .. --build=missing -o silent=True -o ffmpeg=False -o curl=False -o openssl=False -o sdl2=False -o zlib=False`
- `cmake -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 -DDECAF_BUILD_TOOLS=ON -DDECAF_FFMPEG=OFF -DDECAF_VULKAN=OFF -DDECAF_GL=OFF -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DSDL2_DIR=/usr/local/opt/sdl2 -DCMAKE_INSTALL_PREFIX=install ..`
- `make`

Or use `cmake -G "Xcode"` to generate an Xcode project.

## CMake
Options interesting to users:
- DECAF_FFMPEG - Build with ffmpeg which is used for decoding h264 videos
- DECAF_SDL - Build with SDL frontend.
- DECAF_QT - Build with Qt frontend.
- DECAF_VULKAN - Build with Vulkan backend.

Options interesting to developers:
- DECAF_BUILD_TESTS - Build tests.
- DECAF_BUILD_TOOLS - Build tools.
- DECAF_GIT_VERSION - Set this to OFF to disable generating a header with current git version to avoid rebuilding decaf_log.cpp when you do commits locally.
- DECAF_PCH - Enable / disable pch (requires CMake v3.16)
- DECAF_JIT_PROFILING - Build with JIT profiling support.
- DECAF_VALGRIND - Build with Valgrind

## Troubleshooting

decaf-emu builds on github actions CI - so a good reference on how to build is always the CI script itself [.github/workflows/ccpp.yml](https://github.com/decaf-emu/decaf-emu/blob/master/.github/workflows/ccpp.yml)

Often conan requires updating to the latest version to fix various issues, so if it is failing to install dependencies then be sure to `pip install --upgrade conan` / `brew upgrade conan` / update conan manually.

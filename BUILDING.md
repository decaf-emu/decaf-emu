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
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)

Optional:
- [Qt 5.15+ / 6+] (https://www.qt.io/download-qt-installer), disable by using `-DDECAF_QT=OFF`

### Building
- `git clone --recursive https://github.com/decaf-emu/decaf-emu.git`

Use cmake-gui to generate a VS project file:
- Set `Where is the source code` to `[path to decaf-emu.git]`
- Set `Where to build the binaries` to `[path to decaf-emu.git]/build`
- Click `Add Entry` and set `Name: CMAKE_PREFIX_PATH`, `Type: PATH`, `Value` to a Qt5 or Qt6 installation directory, e.g. `Value: C:\Qt\5.15.2\msvc2019_64`
- Click `Configure`
- Ensure `Specify the generator for this project` is set to a version of Visual Studio installed on your computer
- Select `Specify toolchain for cross-compiling`
- Click `Next`
- Set `Specify the toolchain file` to `[path to decaf-emu.git]/libraries/scripts/buildsystems/vcpkg.cmake`
- Click `Finish`
- Configure will run, which may take a while as vcpkg acquires the dependencies, if all works the console should say `Configuring done`
- Click `Generate`, if all works the console should say `Generating done`
- Click `Open Project` to open the generated project in Visual sStudio where you can develop and build.

## Linux

### Dependencies
Required:
- A modern C++17 friendly compiler such as g++9
- CMake

Required dependencies which can be acquired from system or vcpkg:
- c-ares
- curl
- ffmpeg
- libuv
- openssl
- sdl2
- zlib

For some systems, these can be installed with:
- `apt install cmake libcurl4-openssl-dev libsdl2-dev libssl-dev zlib1g-dev libuv1-dev libc-ares-dev libavcodec-dev libavfilter-dev libavutil-dev libswscale-dev`

Optional:
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home), disable by using `-DDECAF_VULKAN=OFF`
- [Qt 5.15+ / 6+] (https://www.qt.io/download-qt-installer), disable by using `-DDECAF_QT=OFF`

For some systems, Qt can be installed with:
- `apt install qtbase5-dev qtbase5-private-dev libqt5svg5-dev libqt5x11extras5-dev mesa-common-dev libglu1-mesa-dev`

### Building
- `git clone --recursive https://github.com/decaf-emu/decaf-emu.git`
- `cd decaf-emu`
- `mkdir build`
- `cd build`
- `cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ../`
- `make`

You might want to use `cmake -G Ninja <...>` and build with Ninja instead of Make for faster builds.

## MacOS
Currently decaf-emu can build on MacOS using Xcode 11 although MoltenVK is missing crucial features which will prevent most games from rendering correctly, e.g. geometry shaders, transform feedback, logic op support, unrestricted depth range. This means the platform should be considered as unsupported.

## CMake
Options interesting to users:
- DECAF_FFMPEG - Build with ffmpeg which is used for decoding h264 videos
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

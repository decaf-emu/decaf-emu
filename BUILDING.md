# Building decaf-emu from source
- [Windows](#windows)
- [Linux](#Linux)
- [MacOS](#MacOS)
- [CMake](#CMake)

## Windows

### Dependencies
Required:
- [Visual Studio 2017](https://visualstudio.microsoft.com/vs/community/)
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
- A modern C++17 friendly compiler such as g++7
- CMake
- libcurl
- openssl
- sdl2
- zlib

Optional:
- ffmpeg, disable by using `-DDECAF_FFMPEG=OFF`
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#windows), disable by using `-DDECAF_VULKAN=OFF`

The dependencies, other than Vulkan, can either be acquired using your package manager such as:
- Required: `apt install cmake libcurl4-openssl-dev libsdl2-dev libssl-dev zlib1g-dev`
- Optional: `apt install libavcodec-dev libavfilter-dev libavutil-dev libswscale-dev`

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
- `cmake -DCMAKE_BUILD_TYPE=Release -DDECAF_QT=ON ../`
- `make`

## MacOS
Right now MacOS is not supported as there are no graphics backends which support MacOS, the OpenGL drivers available on MacOS are too old and we have no Metal backend. MoltenVK has not yet been tested but it is an option to consider. Additionally the latest Xcode as of writing does not support std::filesystem so you must use llvm from brew.

Although I have had success at building with:
- `brew install llvm openssl`
- `export CPPFLAGS="-I/usr/local/opt/llvm/include -I/usr/local/opt/llvm/include/c++/v1"`
- `export LDFLAGS="-L/usr/local/opt/llvm/lib -Wl,-rpath,/usr/local/opt/llvm/lib"`
- `git clone --recursive https://github.com/decaf-emu/decaf-emu.git`
- `cd decaf-emu`
- `mkdir build && cd build`
- `cmake -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DDECAF_GL=OFF -DDECAF_VULKAN=OFF -DDECAF_FFMPEG=OFF ../`
- `make`

## CMake
Options interesting to users:
- DECAF_FFMPEG - Build with ffmpeg which is used for decoding h264 videos.
- DECAF_GL - Build with OpenGL graphics backend.
- DECAF_SDL - Build with SDL frontend.
- DECAF_VULKAN - Build with Vulkan backend.

Options interesting to developers:
- DECAF_BUILD_TESTS - Build tests.
- DECAF_BUILD_TOOLS - Build tools.
- DECAF_BUILD_WUT_TESTS - Build tests which require [wut](https://github.com/decaf-emu/wut).
- DECAF_JIT_ALLOW_PROFILING - Build with JIT profiling SUPPORT.
- DECAF_VALGRIND - Build with Valgrind.

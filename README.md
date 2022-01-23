[![Build status](https://github.com/decaf-emu/decaf-emu/workflows/C%2FC%2B%2B%20CI/badge.svg)](https://github.com/decaf-emu/decaf-emu/actions?workflow=C%2FC%2B%2B+CI)

# decaf-emu
Researching Wii U emulation. Licensed under the terms of the GNU General Public License, version 3 or later (GPLv3+).

You can find us for developer discussion:
- on discord using https://discord.gg/tPqFBnr

<p float="left">
  <img src="https://user-images.githubusercontent.com/1302758/147675484-c0308d89-55a9-4927-8665-1826ee5d4771.png" width="250" />
  <img src="https://user-images.githubusercontent.com/1302758/147674695-d8baf6ac-87e2-487c-8358-ef1588c5e5bf.png" width="250" />
  <img src="https://user-images.githubusercontent.com/1302758/147674704-17767241-e0b4-497e-8841-aa968d14c8e3.png" width="250" />
</p>

## Support
None, this is an in-development project and user support is not provided.

## Building from Source
See [BUILDING.md](BUILDING.md)

## Binaries
The latest Windows and Linux binaries are available via [Actions artifacts](https://github.com/decaf-emu/decaf-emu/actions?query=branch%3Amaster+is%3Asuccess). You must be logged into GitHub in order to download the artifacts.

MacOS builds are currently not provided due to complications with Vulkan.

## Running
Run the `decaf-qt` executable, it is recommended to run the emulator from the root git directory so that it is able to access `resources/fonts/*`.  Alternatively, set `resources_path` in the configuration file to point to the resources directory.

Configuration files can be found at:
- Windows - `%APPDATA%\decaf`
- Linux - `~/.config/decaf`

On Linux, a "Bus error" crash usually indicates an out-of-space error in the temporary directory.  Set the `TMPDIR` environment variable to a directory on a filesystem with at least 2GB free.

Additionally there is an SDL command line application which can be used by `./decaf-sdl play <path to game>`


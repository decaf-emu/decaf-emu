#pragma once
#include <string>
#include <vector>
#include <spdlog\spdlog.h>
#include <glbinding/gl/gl.h>
#include "decaf_input.h"

namespace decaf
{

void
setSystemPath(const std::string &path);

void
setGamePath(const std::string &path);

void
setJitMode(bool enabled,
           bool debug = false);

void
setKernelTraceEnabled(bool enabled);

void
initLogging(std::vector<spdlog::sink_ptr> &sinks,
            spdlog::level::level_enum level);

bool
initialise();

void
start();

void
shutdown();

void
runGpuDriver();

void
shutdownGpuDriver();

void
getSwapBuffers(gl::GLuint* tv,
               gl::GLuint* drc);

float
getAverageFps();

// Stuff for the debugger
void
initialiseDbgUi();

void
drawDbgUi(uint32_t width,
          uint32_t height);

void
injectMouseButtonInput(int button,
                       input::MouseAction action);

void
injectMousePos(float x,
               float y);

void
injectScrollInput(float xoffset,
                  float yoffset);

void
injectKeyInput(input::KeyboardKey key,
               input::KeyboardAction action);

void
injectCharInput(unsigned short c);

using ClipboardTextGetCallback = const char *(*)();
using ClipboardTextSetCallback = void (*)(const char*);

void
setClipboardTextCallbacks(ClipboardTextGetCallback getter,
                          ClipboardTextSetCallback setter);


using VpadSampleCallback = input::ButtonStatus (*)(input::vpad::Channel channel, input::vpad::Core button);

void
setVpadCoreButtonCallback(VpadSampleCallback callback);

} // namespace decaf

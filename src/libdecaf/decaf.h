#pragma once
#include <string>
#include <vector>
#include <spdlog/spdlog.h>
#include <glbinding/gl/gl.h>
#include "decaf_config.h"
#include "decaf_debugger.h"
#include "decaf_input.h"
#include "decaf_graphicsdriver.h"

namespace decaf
{

void
initialiseLogging(std::vector<spdlog::sink_ptr> &sinks,
                  spdlog::level::level_enum level);

bool
initialise(const std::string &gamePath);

void
start();

void
shutdown();

void
setGraphicsDriver(GraphicsDriver *driver);

GraphicsDriver *
getGraphicsDriver();

void
setInputProvider(InputProvider *provider);

InputProvider *
getInputProvider();

// Stuff for the debugger
void
injectMouseButtonInput(input::MouseButton button,
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
injectTextInput(const char *text);

using ClipboardTextGetCallback = const char *(*)();
using ClipboardTextSetCallback = void (*)(const char*);

void
setClipboardTextCallbacks(ClipboardTextGetCallback getter,
                          ClipboardTextSetCallback setter);

} // namespace decaf

#pragma once
#include <string>
#include <vector>
#include <spdlog/spdlog.h>
#include "decaf_config.h"
#include "decaf_debugger.h"
#include "decaf_eventlistener.h"
#include "decaf_graphics.h"
#include "decaf_input.h"

namespace decaf
{

std::string
makeConfigPath(const std::string &filename);

bool
createConfigDirectory();

bool
initialise(const std::string &gamePath);

void
start();

bool
hasExited();

int
waitForExit();

void
shutdown();

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

using ClipboardTextGetCallback = const char *(*)(void *);
using ClipboardTextSetCallback = void (*)(void *, const char*);

void
setClipboardTextCallbacks(ClipboardTextGetCallback getter,
                          ClipboardTextSetCallback setter);

} // namespace decaf

#pragma once
#include "decaf_input.h"
#include <string>

namespace debugger
{

namespace ui
{

void
injectMouseButtonInput(decaf::input::MouseButton button,
                       decaf::input::MouseAction action);

void
injectMousePos(float x,
               float y);

void
injectScrollInput(float xoffset,
                  float yoffset);

void
injectKeyInput(decaf::input::KeyboardKey key,
               decaf::input::KeyboardAction action);

void
injectTextInput(const char *text);

using ClipboardTextGetCallback = const char *(*)();
using ClipboardTextSetCallback = void(*)(const char*);

void
setClipboardTextCallbacks(ClipboardTextGetCallback getter,
                          ClipboardTextSetCallback setter);

void
initialise(const std::string &configPath);

bool
isVisible();

void
updateInput();

void
draw();

} // namespace ui

} // namespace debugger

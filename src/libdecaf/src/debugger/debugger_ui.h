#pragma once
#include "decaf_input.h"
#include "modules/coreinit/coreinit_thread.h"

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
initialise();

void
updateInput();

void
draw();

void
setActiveThread(coreinit::OSThread *thread);

} // namespace ui

} // namespace debugger

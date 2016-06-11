#pragma once
#include "decaf/decaf_input.h"

namespace debugger
{

namespace ui
{

void injectMouseButtonInput(int button, decaf::input::MouseAction action);
void injectMousePos(double x, double y);
void injectScrollInput(double xoffset, double yoffset);
void injectKeyInput(decaf::input::KeyboardKey key, decaf::input::KeyboardAction action);
void injectCharInput(unsigned short c);
void setClipboardTextCallbacks(const char *(*getter)(), void(*setter)(const char*));

void initialise();
void updateInput();
void draw();

} // namespace ui

} // namespace debugger
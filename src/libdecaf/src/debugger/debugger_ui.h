#pragma once
#include "decaf_input.h"

namespace debugger
{

namespace ui
{

using decaf::input::KeyboardAction;
using decaf::input::KeyboardKey;
using decaf::input::MouseAction;
using decaf::input::MouseButton;

bool
onMouseAction(MouseButton button, MouseAction action);

bool
onMouseMove(float x, float y);

bool
onMouseScroll(float x, float y);

bool
onKeyAction(KeyboardKey key, KeyboardAction action);

bool
onText(const char *text);

} // namespace ui

} // namespace debugger

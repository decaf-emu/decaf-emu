#pragma once
#include <string>
#include "input/input.h"

namespace platform
{

using ControllerHandle = uint64_t;

namespace input
{

ControllerHandle
getControllerHandle(const std::string &name);

void
sampleController(ControllerHandle controller);

::input::ButtonStatus
getButtonStatus(ControllerHandle controller, int key);

} // namespace input

} // namespace platform

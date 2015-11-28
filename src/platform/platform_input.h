#pragma once
#include <string>
#include "types.h"
#include "input/input.h"

namespace platform
{

namespace input
{

using ControllerHandle = uint64_t;

ControllerHandle
getControllerHandle(const std::string &name);

void
sampleController(ControllerHandle controller);

::input::ButtonStatus
getButtonStatus(ControllerHandle controller, int key);

} // namespace input

} // namespace platform

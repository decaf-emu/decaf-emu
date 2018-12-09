#pragma once
#include <cstddef>
#include "decaf_input.h"

namespace input
{

namespace vpad = decaf::input::vpad;
namespace wpad = decaf::input::wpad;

void
sampleVpadController(int channel, vpad::Status &status);

void
sampleWpadController(int channel, wpad::Status &status);

} // namespace input

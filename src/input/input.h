#pragma once
#include <cstddef>
#include "decaf/decaf_input.h"

namespace input
{

// TODO: Maybe remove these?
namespace vpad = decaf::input::vpad;
namespace wpad = decaf::input::wpad;
typedef decaf::input::ButtonStatus ButtonStatus;

bool
sampleController(vpad::Channel channel);

void
setVpadCoreButtonCallback(ButtonStatus(vpad::Channel channel, vpad::Core button));

ButtonStatus
getButtonStatus(vpad::Channel channel, vpad::Core button);

ButtonStatus
getButtonStatus(wpad::Channel channel, wpad::Core button);

ButtonStatus
getButtonStatus(wpad::Channel channel, wpad::Nunchuck button);

ButtonStatus
getButtonStatus(wpad::Channel channel, wpad::Classic button);

ButtonStatus
getButtonStatus(wpad::Channel channel, wpad::Pro button);

float
getAxisValue(vpad::Channel channel, vpad::Core axis);

} // namespace input

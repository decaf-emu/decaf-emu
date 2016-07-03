#pragma once
#include <cstddef>
#include "decaf_input.h"

namespace input
{

namespace vpad = decaf::input::vpad;
namespace wpad = decaf::input::wpad;
typedef decaf::input::ButtonStatus ButtonStatus;

vpad::Type
getControllerType(vpad::Channel channel);

ButtonStatus
getButtonStatus(vpad::Channel channel,
                vpad::Core button);

float
getAxisValue(vpad::Channel channel,
             vpad::CoreAxis axis);

bool
getTouchPosition(input::vpad::Channel channel,
                 input::vpad::TouchPosition &position);

wpad::Type
getControllerType(wpad::Channel channel);

ButtonStatus
getButtonStatus(wpad::Channel channel,
                wpad::Core button);

ButtonStatus
getButtonStatus(wpad::Channel channel,
                wpad::Nunchuck button);

ButtonStatus
getButtonStatus(wpad::Channel channel,
                wpad::Classic button);

ButtonStatus
getButtonStatus(wpad::Channel channel,
                wpad::Pro button);

float
getAxisValue(wpad::Channel channel,
             wpad::NunchuckAxis axis);

float
getAxisValue(wpad::Channel channel,
             wpad::ProAxis axis);

} // namespace input

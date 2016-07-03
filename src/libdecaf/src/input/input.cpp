#include "decaf.h"
#include "input.h"

namespace input
{

vpad::Type
getControllerType(vpad::Channel channel)
{
   return decaf::getInputDriver()->getControllerType(channel);
}

ButtonStatus
getButtonStatus(vpad::Channel channel,
                vpad::Core button)
{
   return decaf::getInputDriver()->getButtonStatus(channel, button);
}

float
getAxisValue(vpad::Channel channel,
             vpad::CoreAxis axis)
{
   return decaf::getInputDriver()->getAxisValue(channel, axis);
}

bool
getTouchPosition(input::vpad::Channel channel,
                 input::vpad::TouchPosition &position)
{
   return decaf::getInputDriver()->getTouchPosition(channel, position);
}

wpad::Type
getControllerType(wpad::Channel channel)
{
   return decaf::getInputDriver()->getControllerType(channel);
}

ButtonStatus
getButtonStatus(wpad::Channel channel,
                wpad::Core button)
{
   return decaf::getInputDriver()->getButtonStatus(channel, button);
}

ButtonStatus
getButtonStatus(wpad::Channel channel,
                wpad::Nunchuck button)
{
   return decaf::getInputDriver()->getButtonStatus(channel, button);
}

ButtonStatus
getButtonStatus(wpad::Channel channel,
                wpad::Classic button)
{
   return decaf::getInputDriver()->getButtonStatus(channel, button);
}

ButtonStatus
getButtonStatus(wpad::Channel channel,
                wpad::Pro button)
{
   return decaf::getInputDriver()->getButtonStatus(channel, button);
}

float
getAxisValue(wpad::Channel channel,
             wpad::NunchuckAxis axis)
{
   return decaf::getInputDriver()->getAxisValue(channel, axis);
}

float
getAxisValue(wpad::Channel channel,
             wpad::ProAxis axis)
{
   return decaf::getInputDriver()->getAxisValue(channel, axis);
}

} // namespace input

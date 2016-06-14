#include "decaf.h"
#include "input.h"

namespace input
{

vpad::Type
getControllerType(vpad::Channel channel)
{
   return decaf::getInputProvider()->getControllerType(channel);
}

ButtonStatus
getButtonStatus(vpad::Channel channel,
                vpad::Core button)
{
   return decaf::getInputProvider()->getButtonStatus(channel, button);
}

float
getAxisValue(vpad::Channel channel,
             vpad::CoreAxis axis)
{
   return decaf::getInputProvider()->getAxisValue(channel, axis);
}

wpad::Type
getControllerType(wpad::Channel channel)
{
   return decaf::getInputProvider()->getControllerType(channel);
}

ButtonStatus
getButtonStatus(wpad::Channel channel,
                wpad::Core button)
{
   return decaf::getInputProvider()->getButtonStatus(channel, button);
}

ButtonStatus
getButtonStatus(wpad::Channel channel,
                wpad::Nunchuck button)
{
   return decaf::getInputProvider()->getButtonStatus(channel, button);
}

ButtonStatus
getButtonStatus(wpad::Channel channel,
                wpad::Classic button)
{
   return decaf::getInputProvider()->getButtonStatus(channel, button);
}

ButtonStatus
getButtonStatus(wpad::Channel channel,
                wpad::Pro button)
{
   return decaf::getInputProvider()->getButtonStatus(channel, button);
}

float
getAxisValue(wpad::Channel channel,
             wpad::NunchuckAxis axis)
{
   return decaf::getInputProvider()->getAxisValue(channel, axis);
}

float
getAxisValue(wpad::Channel channel,
             wpad::ProAxis axis)
{
   return decaf::getInputProvider()->getAxisValue(channel, axis);
}

} // namespace input

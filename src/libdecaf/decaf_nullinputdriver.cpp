#include "decaf_nullinputdriver.h"

namespace decaf
{

using namespace input;

// VPAD
vpad::Type
NullInputDriver::getControllerType(vpad::Channel channel)
{
   return vpad::Type::Disconnected;
}

ButtonStatus
NullInputDriver::getButtonStatus(vpad::Channel channel, vpad::Core button)
{
   return ButtonStatus::ButtonReleased;
}

float
NullInputDriver::getAxisValue(vpad::Channel channel, vpad::CoreAxis axis)
{
   return 0.0f;
}

// WPAD
wpad::Type
NullInputDriver::getControllerType(wpad::Channel channel)
{
   return wpad::Type::Disconnected;
}

ButtonStatus
NullInputDriver::getButtonStatus(wpad::Channel channel, wpad::Core button)
{
   return ButtonStatus::ButtonReleased;
}

ButtonStatus
NullInputDriver::getButtonStatus(wpad::Channel channel, wpad::Classic button)
{
   return ButtonStatus::ButtonReleased;
}

ButtonStatus
NullInputDriver::getButtonStatus(wpad::Channel channel, wpad::Nunchuck button)
{
   return ButtonStatus::ButtonReleased;
}

ButtonStatus
NullInputDriver::getButtonStatus(wpad::Channel channel, wpad::Pro button)
{
   return ButtonStatus::ButtonReleased;
}

float
NullInputDriver::getAxisValue(wpad::Channel channel, wpad::NunchuckAxis axis)
{
   return 0.0f;
}

float
NullInputDriver::getAxisValue(wpad::Channel channel, wpad::ProAxis axis)
{
   return 0.0f;
}

} // namespace decaf

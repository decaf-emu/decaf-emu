#include "input.h"

namespace input
{

static ButtonStatus(*sGetVpadCoreButtonStatus)(vpad::Channel channel, vpad::Core button);

void
setVpadCoreButtonCallback(ButtonStatus(*fn)(vpad::Channel channel, vpad::Core button))
{
   sGetVpadCoreButtonStatus = fn;
}

bool
sampleController(vpad::Channel channel)
{
   return true;
}

float
getAxisValue(vpad::Channel channel, vpad::Core id)
{
   return 0.0f;
}

ButtonStatus
getButtonStatus(vpad::Channel channel, vpad::Core button)
{
   return sGetVpadCoreButtonStatus(channel, button);
}

ButtonStatus
getButtonStatus(wpad::Channel channel, wpad::Core button)
{
   return ButtonStatus::ButtonReleased;
}

ButtonStatus
getButtonStatus(wpad::Channel channel, wpad::Nunchuck button)
{
   return ButtonStatus::ButtonReleased;
}

ButtonStatus
getButtonStatus(wpad::Channel channel, wpad::Classic button)
{
   return ButtonStatus::ButtonReleased;
}

ButtonStatus
getButtonStatus(wpad::Channel channel, wpad::Pro button)
{
   return ButtonStatus::ButtonReleased;
}

} // namespace input

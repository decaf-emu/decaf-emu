#include "input.h"

namespace input
{

static VpadSampleCallback
sGetVpadCoreButtonStatus = nullptr;

void
setVpadCoreButtonCallback(VpadSampleCallback callback)
{
   sGetVpadCoreButtonStatus = callback;
}

bool
sampleController(vpad::Channel channel)
{
   return true;
}

float
getAxisValue(vpad::Channel channel,
             vpad::Core id)
{
   return 0.0f;
}

ButtonStatus
getButtonStatus(vpad::Channel channel,
                vpad::Core button)
{
   if (sGetVpadCoreButtonStatus) {
      return sGetVpadCoreButtonStatus(channel, button);
   } else {
      return ButtonStatus::ButtonReleased;
   }
}

ButtonStatus
getButtonStatus(wpad::Channel channel,
                wpad::Core button)
{
   return ButtonStatus::ButtonReleased;
}

ButtonStatus
getButtonStatus(wpad::Channel channel,
                wpad::Nunchuck button)
{
   return ButtonStatus::ButtonReleased;
}

ButtonStatus
getButtonStatus(wpad::Channel channel,
                wpad::Classic button)
{
   return ButtonStatus::ButtonReleased;
}

ButtonStatus
getButtonStatus(wpad::Channel channel,
                wpad::Pro button)
{
   return ButtonStatus::ButtonReleased;
}

} // namespace input

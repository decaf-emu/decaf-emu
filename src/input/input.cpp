#include "input.h"
#include "config.h"
#include "platform/platform_input.h"

namespace input
{

static platform::input::ControllerHandle
gVpadHandles[vpad::MaxControllers];

static platform::input::ControllerHandle
gWpadHandles[wpad::MaxControllers];

static int
getButtonMapping(vpad::Channel channel, vpad::Core button)
{
   switch (button) {
   case vpad::Core::Up:
      return config::input::vpad0::button_up;
   case vpad::Core::Down:
      return config::input::vpad0::button_down;
   case vpad::Core::Left:
      return config::input::vpad0::button_left;
   case vpad::Core::Right:
      return config::input::vpad0::button_right;
   case vpad::Core::A:
      return config::input::vpad0::button_a;
   case vpad::Core::B:
      return config::input::vpad0::button_b;
   case vpad::Core::X:
      return config::input::vpad0::button_x;
   case vpad::Core::Y:
      return config::input::vpad0::button_y;
   case vpad::Core::TriggerR:
      return config::input::vpad0::button_trigger_r;
   case vpad::Core::TriggerL:
      return config::input::vpad0::button_trigger_l;
   case vpad::Core::TriggerZR:
      return config::input::vpad0::button_trigger_zr;
   case vpad::Core::TriggerZL:
      return config::input::vpad0::button_trigger_zl;
   case vpad::Core::StickL:
      return config::input::vpad0::button_stick_l;
   case vpad::Core::StickR:
      return config::input::vpad0::button_stick_r;
   case vpad::Core::Plus:
      return config::input::vpad0::button_plus;
   case vpad::Core::Minus:
      return config::input::vpad0::button_minus;
   case vpad::Core::Home:
      return config::input::vpad0::button_home;
   case vpad::Core::Sync:
      return config::input::vpad0::button_sync;
   }

   return 0;
}

static platform::input::ControllerHandle
getControllerHandle(vpad::Channel channel)
{
   auto id = static_cast<size_t>(channel);

   if (id >= vpad::MaxControllers) {
      return 0;
   } else {
      return gVpadHandles[id];
   }
}

static platform::input::ControllerHandle
getControllerHandle(wpad::Channel channel)
{
   auto id = static_cast<size_t>(channel);

   if (id >= wpad::MaxControllers) {
      return 0;
   } else {
      return gWpadHandles[id];
   }
}

bool
init()
{
   gVpadHandles[0] = platform::input::getControllerHandle(config::input::vpad0::name);

   // TODO: Config 4 wpad controllers
   for (auto i = 0; i < 4; ++i) {
      gWpadHandles[i] = 0;
   }

   return true;
}

bool
sampleController(vpad::Channel channel)
{
   platform::input::sampleController(getControllerHandle(channel));
   return true;
}

ButtonStatus
getButtonStatus(vpad::Channel channel, vpad::Core button)
{
   auto key = getButtonMapping(channel, button);
   auto controller = getControllerHandle(channel);
   return platform::input::getButtonStatus(controller, key);
}

ButtonStatus
getButtonStatus(wpad::Channel channel, wpad::Core button)
{
   return ButtonReleased;
}

ButtonStatus
getButtonStatus(wpad::Channel channel, wpad::Nunchuck button)
{
   return ButtonReleased;
}

ButtonStatus
getButtonStatus(wpad::Channel channel, wpad::Classic button)
{
   return ButtonReleased;
}

ButtonStatus
getButtonStatus(wpad::Channel channel, wpad::Pro button)
{
   return ButtonReleased;
}

} // namespace input

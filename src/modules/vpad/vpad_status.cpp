#include "vpad.h"
#include "vpad_status.h"
#include "input/input.h"
#include <vector>
#include <utility>

namespace vpad
{

static const std::vector<std::pair<Buttons::Buttons, input::vpad::Core>>
gButtonMap =
{
   { Buttons::Sync,     input::vpad::Core::Sync },
   { Buttons::Home,     input::vpad::Core::Home },
   { Buttons::Minus,    input::vpad::Core::Minus },
   { Buttons::Plus,     input::vpad::Core::Plus },
   { Buttons::R,        input::vpad::Core::TriggerR },
   { Buttons::L,        input::vpad::Core::TriggerL },
   { Buttons::ZR,       input::vpad::Core::TriggerZR },
   { Buttons::ZL,       input::vpad::Core::TriggerZL },
   { Buttons::Down,     input::vpad::Core::Down },
   { Buttons::Up,       input::vpad::Core::Up },
   { Buttons::Left,     input::vpad::Core::Left },
   { Buttons::Right,    input::vpad::Core::Right },
   { Buttons::Y,        input::vpad::Core::Y },
   { Buttons::X,        input::vpad::Core::X },
   { Buttons::B,        input::vpad::Core::B },
   { Buttons::A,        input::vpad::Core::A },
   { Buttons::StickL,   input::vpad::Core::LeftStick },
   { Buttons::StickR,   input::vpad::Core::RightStick },
};

static uint32_t
gLastButtonState = 0;

int32_t
VPADRead(uint32_t chan, VPADStatus *buffers, uint32_t count, be_val<VpadReadError::Error> *error)
{
   assert(count >= 1);

   if (chan >= input::vpad::MaxControllers) {
      if (error) {
         *error = VpadReadError::InvalidController;
      }

      return 0;
   }

   memset(&buffers[0], 0, sizeof(VPADStatus));

   auto channel = static_cast<input::vpad::Channel>(chan);
   input::sampleController(channel);

   auto &buffer = buffers[0];

   // Update button state
   for (auto &pair : gButtonMap) {
      auto bit = pair.first;
      auto button = pair.second;
      auto status = input::getButtonStatus(channel, button);
      auto previous = gLastButtonState & bit;

      if (status == input::ButtonStatus::ButtonPressed) {
         if (!previous) {
            buffer.trigger |= bit;
         }

         buffer.hold |= bit;
      } else if (previous) {
         buffer.release |= bit;
      }
   }

   gLastButtonState = buffer.hold;

   // Update axis state
   buffer.leftStick.x = input::getAxisValue(channel, input::vpad::Core::LeftStickX);
   buffer.leftStick.y = input::getAxisValue(channel, input::vpad::Core::LeftStickY);
   buffer.rightStick.x = input::getAxisValue(channel, input::vpad::Core::RightStickX);
   buffer.rightStick.y = input::getAxisValue(channel, input::vpad::Core::RightStickY);

   if (error) {
      *error = VpadReadError::Success;
   }

   return 1;
}

void
Module::registerStatusFunctions()
{
   RegisterKernelFunction(VPADRead);
}

} // namespace vpad

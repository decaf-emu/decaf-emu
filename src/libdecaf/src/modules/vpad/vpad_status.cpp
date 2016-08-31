#include "vpad.h"
#include "vpad_status.h"
#include "input/input.h"
#include <vector>
#include <utility>

namespace vpad
{

static const std::vector<std::pair<Buttons, input::vpad::Core>>
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
VPADRead(uint32_t chan,
         VPADStatus *buffers,
         uint32_t count,
         be_val<VPADReadError> *error)
{
   if (count < 1) {
      if (error) {
         *error = VPADReadError::NoSamples;
      }

      return 0;
   }

   if (chan >= input::vpad::MaxControllers) {
      if (error) {
         *error = VPADReadError::InvalidController;
      }

      return 0;
   }

   memset(&buffers[0], 0, sizeof(VPADStatus));

   auto channel = static_cast<input::vpad::Channel>(chan);
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
   buffer.leftStick.x = input::getAxisValue(channel, input::vpad::CoreAxis::LeftStickX);
   buffer.leftStick.y = input::getAxisValue(channel, input::vpad::CoreAxis::LeftStickY);
   buffer.rightStick.x = input::getAxisValue(channel, input::vpad::CoreAxis::RightStickX);
   buffer.rightStick.y = input::getAxisValue(channel, input::vpad::CoreAxis::RightStickY);

   // Update touchpad data
   input::vpad::TouchPosition position;

   if (input::getTouchPosition(channel, position)) {
      buffer.tpNormal.touched = 1;
      buffer.tpNormal.x = static_cast<uint16_t>(position.x * 1280.0f);
      buffer.tpNormal.y = static_cast<uint16_t>(position.y * 720.0f);
      buffer.tpNormal.validity = vpad::TouchPadValidity::Valid;

      // For now, lets just copy instantaneous position tpNormal to tpFiltered.
      // My guess is that tpFiltered1/2 "filter" results over a period of time
      // to allow for smoother input, due to the fact that touch screens aren't
      // super precise and people's fingers are fat. I would guess tpFiltered1
      // is filtered over a shorter period and tpFiltered2 over a longer period.
      buffer.tpFiltered1 = buffer.tpNormal;
      buffer.tpFiltered2 = buffer.tpNormal;
   }

   if (error) {
      *error = VPADReadError::Success;
   }

   return 1;
}

void
VPADGetTPCalibratedPoint(uint32_t chan,
                         VPADTouchData *calibratedData,
                         VPADTouchData *uncalibratedData)
{
   // TODO: Actually I think we are meant to adjust uncalibratedData based
   // off of what is set by VPADSetTPCalibrationParam
   std::memcpy(calibratedData, uncalibratedData, sizeof(VPADTouchData));
}

bool
VPADBASEGetHeadphoneStatus(uint32_t chan)
{
   return false;
}

void
Module::registerStatusFunctions()
{
   RegisterKernelFunction(VPADRead);
   RegisterKernelFunction(VPADGetTPCalibratedPoint);
   RegisterKernelFunction(VPADBASEGetHeadphoneStatus);
}

} // namespace vpad

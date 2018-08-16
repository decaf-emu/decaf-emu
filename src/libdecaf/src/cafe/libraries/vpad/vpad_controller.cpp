#include "vpad.h"
#include "vpad_controller.h"
#include "input/input.h"

#include <vector>
#include <utility>

namespace cafe::vpad
{

static const std::vector<std::pair<VPADButtons, input::vpad::Core>>
gButtonMap =
{
   { VPADButtons::Sync,     input::vpad::Core::Sync },
   { VPADButtons::Home,     input::vpad::Core::Home },
   { VPADButtons::Minus,    input::vpad::Core::Minus },
   { VPADButtons::Plus,     input::vpad::Core::Plus },
   { VPADButtons::R,        input::vpad::Core::TriggerR },
   { VPADButtons::L,        input::vpad::Core::TriggerL },
   { VPADButtons::ZR,       input::vpad::Core::TriggerZR },
   { VPADButtons::ZL,       input::vpad::Core::TriggerZL },
   { VPADButtons::Down,     input::vpad::Core::Down },
   { VPADButtons::Up,       input::vpad::Core::Up },
   { VPADButtons::Left,     input::vpad::Core::Left },
   { VPADButtons::Right,    input::vpad::Core::Right },
   { VPADButtons::Y,        input::vpad::Core::Y },
   { VPADButtons::X,        input::vpad::Core::X },
   { VPADButtons::B,        input::vpad::Core::B },
   { VPADButtons::A,        input::vpad::Core::A },
   { VPADButtons::StickL,   input::vpad::Core::LeftStick },
   { VPADButtons::StickR,   input::vpad::Core::RightStick },
};

static uint32_t
gLastButtonState = 0;

void
VPADInit()
{
}

void
VPADSetAccParam(VPADChan chan,
                float unk1,
                float unk2)
{
}

void
VPADSetBtnRepeat(VPADChan chan,
                 float unk1,
                 float unk2)
{
}


/**
 * VPADRead
 *
 * \return
 * Returns the number of samples read.
 */
uint32_t
VPADRead(VPADChan chan,
         virt_ptr<VPADStatus> buffers,
         uint32_t bufferCount,
         virt_ptr<VPADReadError> outError)
{
   if (bufferCount < 1) {
      if (outError) {
         *outError = VPADReadError::NoSamples;
      }

      return 0;
   }

   if (chan >= input::vpad::MaxControllers) {
      if (outError) {
         *outError = VPADReadError::InvalidController;
      }

      return 0;
   }

   memset(virt_addrof(buffers[0]).getRawPointer(), 0, sizeof(VPADStatus));

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
      buffer.tpNormal.touched = uint16_t { 1 };
      buffer.tpNormal.x = static_cast<uint16_t>(position.x * 1280.0f);
      buffer.tpNormal.y = static_cast<uint16_t>(position.y * 720.0f);
      buffer.tpNormal.validity = VPADTouchPadValidity::Valid;

      // For now, lets just copy instantaneous position tpNormal to tpFiltered.
      // My guess is that tpFiltered1/2 "filter" results over a period of time
      // to allow for smoother input, due to the fact that touch screens aren't
      // super precise and people's fingers are fat. I would guess tpFiltered1
      // is filtered over a shorter period and tpFiltered2 over a longer period.
      buffer.tpFiltered1 = buffer.tpNormal;
      buffer.tpFiltered2 = buffer.tpNormal;
   }

   if (outError) {
      *outError = VPADReadError::Success;
   }

   return 1;
}

void
VPADGetTPCalibratedPoint(VPADChan chan,
                         virt_ptr<VPADTouchData> calibratedData,
                         virt_ptr<VPADTouchData> uncalibratedData)
{
   // TODO: Actually I think we are meant to adjust uncalibratedData based
   // off of what is set by VPADSetTPCalibrationParam
   std::memcpy(calibratedData.getRawPointer(),
               uncalibratedData.getRawPointer(),
               sizeof(VPADTouchData));
}

bool
VPADBASEGetHeadphoneStatus(VPADChan chan)
{
   return false;
}

void
Library::registerControllerSymbols()
{
   RegisterFunctionExport(VPADInit);
   RegisterFunctionExport(VPADSetAccParam);
   RegisterFunctionExport(VPADSetBtnRepeat);
   RegisterFunctionExport(VPADRead);
   RegisterFunctionExport(VPADGetTPCalibratedPoint);
   RegisterFunctionExport(VPADBASEGetHeadphoneStatus);
}

} // namespace cafe::vpad

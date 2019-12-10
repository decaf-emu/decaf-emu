#include "vpad.h"
#include "vpad_controller.h"
#include "input/input.h"

#include <vector>
#include <utility>

namespace cafe::vpad
{

struct StaticControllerData
{
   be2_array<VPADTouchCalibrationParam, 2> calibrationParam {
      VPADTouchCalibrationParam {
         uint16_t { 0 }, uint16_t { 0 }, 1280.0f / 4096.0f, 720.0f / 4096.0f
      },
      VPADTouchCalibrationParam {
         uint16_t { 0 }, uint16_t { 0 }, 1280.0f / 4096.0f, 720.0f / 4096.0f
      }
   };
   be2_val<VPADButtons> lastButtonState = VPADButtons { 0 };
};

static virt_ptr<StaticControllerData> sControllerData = nullptr;

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

   if (chan >= VPADChan::Max) {
      if (outError) {
         *outError = VPADReadError::InvalidController;
      }

      return 0;
   }

   memset(virt_addrof(buffers[0]).get(), 0, sizeof(VPADStatus));

   auto status = input::vpad::Status { };
   input::sampleVpadController(static_cast<int>(chan), status);

   if (!status.connected) {
      if (outError) {
         *outError = VPADReadError::InvalidController;
      }

      return 0;
   }

   auto &buffer = buffers[0];
   auto hold = VPADButtons { 0 };
   if (status.buttons.sync) { hold |= VPADButtons::Sync; }
   if (status.buttons.home) { hold |= VPADButtons::Home; }
   if (status.buttons.minus) { hold |= VPADButtons::Minus; }
   if (status.buttons.plus) { hold |= VPADButtons::Plus; }
   if (status.buttons.r) { hold |= VPADButtons::R; }
   if (status.buttons.l) { hold |= VPADButtons::L; }
   if (status.buttons.zr) { hold |= VPADButtons::ZR; }
   if (status.buttons.zl) { hold |= VPADButtons::ZL; }
   if (status.buttons.down) { hold |= VPADButtons::Down; }
   if (status.buttons.up) { hold |= VPADButtons::Up; }
   if (status.buttons.right) { hold |= VPADButtons::Right; }
   if (status.buttons.left) { hold |= VPADButtons::Left; }
   if (status.buttons.x) { hold |= VPADButtons::X; }
   if (status.buttons.y) { hold |= VPADButtons::Y; }
   if (status.buttons.b) { hold |= VPADButtons::B; }
   if (status.buttons.a) { hold |= VPADButtons::A; }
   if (status.buttons.stickR) { hold |= VPADButtons::StickR; }
   if (status.buttons.stickL) { hold |= VPADButtons::StickL; }

   buffer.hold = hold;
   buffer.trigger = (~sControllerData->lastButtonState) & hold;
   buffer.release = sControllerData->lastButtonState & (~hold);
   sControllerData->lastButtonState = hold;

   // Update axis state
   buffer.leftStick.x = status.leftStickX;
   buffer.leftStick.y = status.leftStickY;
   buffer.rightStick.x = status.rightStickX;
   buffer.rightStick.y = status.rightStickY;

   // Update touchpad data
   if (status.touch.down) {
      buffer.tpNormal.touched = uint16_t { 1 };
      buffer.tpNormal.x = static_cast<uint16_t>(status.touch.x * 1280.0f);
      buffer.tpNormal.y = static_cast<uint16_t>(status.touch.y * 720.0f);
      buffer.tpNormal.validity = VPADTouchPadValidity::Valid;
   } else {
      buffer.tpNormal.touched = uint16_t { 0 };
      buffer.tpNormal.validity = VPADTouchPadValidity::InvalidX | VPADTouchPadValidity::InvalidY;
   }

   // For now, lets just copy instantaneous position tpNormal to tpFiltered.
   // My guess is that tpFiltered1/2 "filter" results over a period of time
   // to allow for smoother input, due to the fact that touch screens aren't
   // super precise and people's fingers are fat. I would guess tpFiltered1
   // is filtered over a shorter period and tpFiltered2 over a longer period.
   buffer.tpFiltered1 = buffer.tpNormal;
   buffer.tpFiltered2 = buffer.tpNormal;

   if (outError) {
      *outError = VPADReadError::Success;
   }

   return 1;
}

void
VPADGetTPCalibrationParam(VPADChan chan,
                          virt_ptr<VPADTouchCalibrationParam> outParam)
{
   *outParam = sControllerData->calibrationParam[chan];
}

void
VPADGetTPCalibratedPoint(VPADChan chan,
                         virt_ptr<VPADTouchData> calibratedData,
                         virt_ptr<VPADTouchData> uncalibratedData)
{
   // TODO: Actually I think we are meant to adjust uncalibratedData based
   // off of what is set by VPADSetTPCalibrationParam
   std::memcpy(calibratedData.get(),
               uncalibratedData.get(),
               sizeof(VPADTouchData));
}

void
VPADGetTPCalibratedPointEx(VPADChan chan,
                           VPADTouchPadResolution tpReso,
                           virt_ptr<VPADTouchData> calibratedData,
                           virt_ptr<VPADTouchData> uncalibratedData)
{
   // TODO: Actually use per resolution calibrated data
   VPADGetTPCalibratedPoint(chan, calibratedData, uncalibratedData);
}

void
VPADSetTPCalibrationParam(VPADChan chan,
                          virt_ptr<const VPADTouchCalibrationParam> param)
{
   sControllerData->calibrationParam[chan] = *param;
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
   RegisterFunctionExport(VPADGetTPCalibrationParam);
   RegisterFunctionExport(VPADGetTPCalibratedPoint);
   RegisterFunctionExport(VPADGetTPCalibratedPointEx);
   RegisterFunctionExport(VPADSetTPCalibrationParam);
   RegisterFunctionExport(VPADBASEGetHeadphoneStatus);

   RegisterDataInternal(sControllerData);
}

} // namespace cafe::vpad

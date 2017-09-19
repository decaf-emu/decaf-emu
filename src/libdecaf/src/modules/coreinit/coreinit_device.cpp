#include "coreinit.h"
#include "coreinit_device.h"
#include "coreinit_enum_string.h"
#include <common/bitfield.h>
#include "modules/vpad/vpad_status.h"

static bool
sEnableVPADDevice = false;

namespace coreinit
{

static SIDevice
sInputDevice = { 0 };

void
OSEnforceInorderIO()
{
   // TODO: OSEnforceInorderIO
}

uint16_t
OSReadRegister16(OSDeviceID device,
                 uint32_t id)
{
   return OSReadRegister32Ex(device, id) & 0xFFFF;
}

uint32_t
OSReadRegister32Ex(OSDeviceID device,
                   uint32_t id)
{
   if (device == OSDeviceID::SI) {
      auto reg = static_cast<SIRegisters>(id);

      auto status0 = SIDevice::ControllerStatus0::get(0)
         .stickLX(128)
         .stickLY(128)
         .error(false);

      auto status1 = SIDevice::ControllerStatus1::get(0)
         .stickRX(128)
         .stickRY(128);

      if (sEnableVPADDevice) {
         if (reg == SIRegisters::Controller0Status0
          || reg == SIRegisters::Controller0Status1) {
            vpad::VPADStatus status;

            if (vpad::VPADRead(0, &status, 1, nullptr) == 1) {
               // VPAD sticks are -1.0f to 1.0f, whereas these sticks are 0 - 255
               auto convertStickValue = [](float value) {
                  return static_cast<uint8_t>(std::min(static_cast<unsigned int>(((value + 1.0f) / 2.0f) * 256.0f), 255u));
               };

               status0 = status0
                  .btnA(!!(status.hold & vpad::Buttons::A))
                  .btnB(!!(status.hold & vpad::Buttons::B))
                  .btnX(!!(status.hold & vpad::Buttons::X))
                  .btnY(!!(status.hold & vpad::Buttons::Y))
                  .btnPlus(!!(status.hold & vpad::Buttons::Plus))
                  .btnLeft(!!(status.hold & vpad::Buttons::Left))
                  .btnRight(!!(status.hold & vpad::Buttons::Right))
                  .btnDown(!!(status.hold & vpad::Buttons::Down))
                  .btnUp(!!(status.hold & vpad::Buttons::Up))
                  .btnTriggerL(!!(status.hold & vpad::Buttons::L))
                  .btnTriggerR(!!(status.hold & vpad::Buttons::R))
                  .btnTriggerZ(!!(status.hold & vpad::Buttons::ZR))
                  .stickLX(convertStickValue(status.leftStick.x))
                  .stickLY(convertStickValue(status.leftStick.y))
                  .error(false);

               status1 = status1
                  .stickRX(convertStickValue(status.rightStick.x))
                  .stickRY(convertStickValue(status.rightStick.y));
            }
         }
      }

      switch (reg) {
      case SIRegisters::DeviceStatus:
         return sInputDevice.deviceStatus.value;
      case SIRegisters::ControllerError:
         return sInputDevice.controllerError.value;
      case SIRegisters::PollControl:
         return sInputDevice.pollControl.value;
      case SIRegisters::Controller0Command:
         return sInputDevice.controllers[0].command.value;
      case SIRegisters::Controller1Command:
         return sInputDevice.controllers[1].command.value;
      case SIRegisters::Controller2Command:
         return sInputDevice.controllers[2].command.value;
      case SIRegisters::Controller3Command:
         return sInputDevice.controllers[3].command.value;
      case SIRegisters::Controller0Status0:
      case SIRegisters::Controller1Status0:
      case SIRegisters::Controller2Status0:
      case SIRegisters::Controller3Status0:
         return status0.value;
      case SIRegisters::Controller0Status1:
      case SIRegisters::Controller1Status1:
      case SIRegisters::Controller2Status1:
      case SIRegisters::Controller3Status1:
         return status1.value;
      default:
         gLog->warn("OSReadRegister - Unimplemented device {} register {}", to_string(device), to_string(reg));
         return 0;
      }
   }

   gLog->warn("OSReadRegister - Unimplemented device {} register {}", to_string(device), id);
   return 0;
}

void
OSWriteRegister32Ex(OSDeviceID device,
                    uint32_t id,
                    uint32_t value)
{
   if (device == OSDeviceID::SI) {
      auto reg = static_cast<SIRegisters>(id);

      switch (id) {
      case SIRegisters::Controller0Command:
         sInputDevice.controllers[0].command = SIDevice::ControllerCommand::get(value);
         break;
      case SIRegisters::Controller1Command:
         sInputDevice.controllers[1].command = SIDevice::ControllerCommand::get(value);
         break;
      case SIRegisters::Controller2Command:
         sInputDevice.controllers[2].command = SIDevice::ControllerCommand::get(value);
         break;
      case SIRegisters::Controller3Command:
         sInputDevice.controllers[3].command = SIDevice::ControllerCommand::get(value);
         break;
      case SIRegisters::PollControl:
         sInputDevice.pollControl = SIDevice::PollControl::get(value);
         break;
      case SIRegisters::DeviceStatus:
         sInputDevice.deviceStatus = SIDevice::DeviceStatus::get(value);
         break;
      case SIRegisters::ControllerError:
         sInputDevice.controllerError = SIDevice::ControllerError::get(value);
         break;
      default:
         gLog->warn("OSWriteRegister32 - Unimplemented device {} register {} = 0x{:08X}", to_string(device), to_string(reg), value);
      }
   } else {
      gLog->warn("OSWriteRegister32 - Unimplemented device {} register {} = 0x{:08X}", to_string(device), id, value);
   }
}

void
OSWriteRegister16(OSDeviceID device,
                  uint32_t id,
                  uint16_t value)
{
   gLog->warn("OSWriteRegister16 - Unimplemented device {} register {} = 0x{:08X}", to_string(device), id, value);
}

void
Module::registerDeviceFunctions()
{
   RegisterKernelFunction(OSReadRegister16);
   RegisterKernelFunction(OSWriteRegister16);
   RegisterKernelFunction(OSEnforceInorderIO);
   RegisterKernelFunctionName("__OSReadRegister32Ex", OSReadRegister32Ex);
   RegisterKernelFunctionName("__OSWriteRegister32Ex", OSWriteRegister32Ex);
}

} // namespace coreinit

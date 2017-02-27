#include "coreinit.h"
#include "coreinit_device.h"
#include "coreinit_enum_string.h"
#include <common/bitfield.h>
#include "modules/vpad/vpad_status.h"

static bool
sEnableVPADDevice = false;

namespace coreinit
{

static OSInputDevice
sInputDevice = { 0 };

static BOOL
OSDriver_Register(uint32_t r3,
                  uint32_t r4,
                  void *r5,
                  uint32_t r6,
                  uint32_t r7,
                  uint32_t *r8,
                  uint32_t r9)
{
   // TODO: OSDriver_Register
   return FALSE;
}

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
   if (device == OSDeviceID::Input) {
      auto reg = static_cast<OSDeviceInputRegisters>(id);

      auto status0 = OSInputDevice::ControllerStatus0::get(0)
         .stickLX(128)
         .stickLY(128)
         .error(false);

      auto status1 = OSInputDevice::ControllerStatus1::get(0)
         .stickRX(128)
         .stickRY(128);

      if (sEnableVPADDevice) {
         if (reg == OSDeviceInputRegisters::Controller0Status0
          || reg == OSDeviceInputRegisters::Controller0Status1) {
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
      case OSDeviceInputRegisters::DeviceStatus:
         return sInputDevice.deviceStatus.value;
      case OSDeviceInputRegisters::ControllerError:
         return sInputDevice.controllerError.value;
      case OSDeviceInputRegisters::PollControl:
         return sInputDevice.pollControl.value;
      case OSDeviceInputRegisters::Controller0Command:
         return sInputDevice.controllers[0].command.value;
      case OSDeviceInputRegisters::Controller1Command:
         return sInputDevice.controllers[1].command.value;
      case OSDeviceInputRegisters::Controller2Command:
         return sInputDevice.controllers[2].command.value;
      case OSDeviceInputRegisters::Controller3Command:
         return sInputDevice.controllers[3].command.value;
      case OSDeviceInputRegisters::Controller0Status0:
      case OSDeviceInputRegisters::Controller1Status0:
      case OSDeviceInputRegisters::Controller2Status0:
      case OSDeviceInputRegisters::Controller3Status0:
         return status0.value;
      case OSDeviceInputRegisters::Controller0Status1:
      case OSDeviceInputRegisters::Controller1Status1:
      case OSDeviceInputRegisters::Controller2Status1:
      case OSDeviceInputRegisters::Controller3Status1:
         return status1.value;
      default:
         gLog->warn("OSReadRegister - Unimplemented device {} register {}", enumAsString(device), enumAsString(reg));
         return 0;
      }
   }

   gLog->warn("OSReadRegister - Unimplemented device {} register {}", enumAsString(device), id);
   return 0;
}

void
OSWriteRegister32Ex(OSDeviceID device,
                    uint32_t id,
                    uint32_t value)
{
   if (device == OSDeviceID::Input) {
      auto reg = static_cast<OSDeviceInputRegisters>(id);

      switch (id) {
      case OSDeviceInputRegisters::Controller0Command:
         sInputDevice.controllers[0].command = OSInputDevice::ControllerCommand::get(value);
         break;
      case OSDeviceInputRegisters::Controller1Command:
         sInputDevice.controllers[1].command = OSInputDevice::ControllerCommand::get(value);
         break;
      case OSDeviceInputRegisters::Controller2Command:
         sInputDevice.controllers[2].command = OSInputDevice::ControllerCommand::get(value);
         break;
      case OSDeviceInputRegisters::Controller3Command:
         sInputDevice.controllers[3].command = OSInputDevice::ControllerCommand::get(value);
         break;
      case OSDeviceInputRegisters::PollControl:
         sInputDevice.pollControl = OSInputDevice::PollControl::get(value);
         break;
      case OSDeviceInputRegisters::DeviceStatus:
         sInputDevice.deviceStatus = OSInputDevice::DeviceStatus::get(value);
         break;
      case OSDeviceInputRegisters::ControllerError:
         sInputDevice.controllerError = OSInputDevice::ControllerError::get(value);
         break;
      default:
         gLog->warn("OSWriteRegister32 - Unimplemented device {} register {} = 0x{:08X}", enumAsString(device), enumAsString(reg), value);
      }
   } else {
      gLog->warn("OSWriteRegister32 - Unimplemented device {} register {} = 0x{:08X}", enumAsString(device), id, value);
   }
}

void
OSWriteRegister16(OSDeviceID device,
                  uint32_t id,
                  uint16_t value)
{
   gLog->warn("OSWriteRegister16 - Unimplemented device {} register {} = 0x{:08X}", enumAsString(device), id, value);
}

void
Module::registerDeviceFunctions()
{
   RegisterKernelFunction(OSDriver_Register);
   RegisterKernelFunction(OSReadRegister16);
   RegisterKernelFunction(OSWriteRegister16);
   RegisterKernelFunction(OSEnforceInorderIO);
   RegisterKernelFunctionName("__OSReadRegister32Ex", OSReadRegister32Ex);
   RegisterKernelFunctionName("__OSWriteRegister32Ex", OSWriteRegister32Ex);
}

} // namespace coreinit

#include "coreinit.h"
#include "coreinit_device.h"
#include "coreinit_enum_string.h"
#include "common/bitfield.h"

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
         return OSInputDevice::ControllerStatus0::get(0)
            .error(false)
            .stickLX(-0x80)
            .stickLY(-0x80)
            .value;
      case OSDeviceInputRegisters::Controller0Status1:
      case OSDeviceInputRegisters::Controller1Status1:
      case OSDeviceInputRegisters::Controller2Status1:
      case OSDeviceInputRegisters::Controller3Status1:
         return OSInputDevice::ControllerStatus1::get(0)
            .stickRX(-0x80)
            .stickRY(-0x80)
            .value;
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
         gLog->warn("OSWriteRegister - Unimplemented device {} register {} = 0x{:08X}", enumAsString(device), enumAsString(reg), value);
      }
   } else {
      gLog->warn("OSWriteRegister - Unimplemented device {} register {} = 0x{:08X}", enumAsString(device), id, value);
   }
}

void
Module::registerDeviceFunctions()
{
   RegisterKernelFunction(OSDriver_Register);
   RegisterKernelFunction(OSReadRegister16);
   RegisterKernelFunction(OSEnforceInorderIO);
   RegisterKernelFunctionName("__OSReadRegister32Ex", OSReadRegister32Ex);
   RegisterKernelFunctionName("__OSWriteRegister32Ex", OSWriteRegister32Ex);
}

} // namespace coreinit

#pragma once
#include "coreinit_enum.h"

#include <common/bitfield.h>
#include <common/cbool.h>
#include <cstdint>

namespace coreinit
{

/**
 * \defgroup coreinit_device Device
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

struct SIDevice
{
   BITFIELD(DeviceStatus, uint32_t)
      BITFIELD_ENTRY(0, 1, bool, device_busy);
   BITFIELD_END

   BITFIELD(PollControl, uint32_t)
      BITFIELD_ENTRY(4, 1, bool, pollController3);
      BITFIELD_ENTRY(5, 1, bool, pollController2);
      BITFIELD_ENTRY(6, 1, bool, pollController1);
      BITFIELD_ENTRY(7, 1, bool, pollController0);
   BITFIELD_END

   BITFIELD(ControllerError, uint32_t)
      BITFIELD_ENTRY(0, 4, uint8_t, controllerError3);
      BITFIELD_ENTRY(8, 4, uint8_t, controllerError2);
      BITFIELD_ENTRY(16, 4, uint8_t, controllerError1);
      BITFIELD_ENTRY(24, 4, uint8_t, controllerError0);
   BITFIELD_END

   BITFIELD(ControllerCommand, uint32_t)
   BITFIELD_END

   BITFIELD(ControllerStatus0, uint32_t)
      BITFIELD_ENTRY(0, 8, uint8_t, stickLY);
      BITFIELD_ENTRY(8, 8, uint8_t, stickLX);
      BITFIELD_ENTRY(16, 1, bool, btnLeft);
      BITFIELD_ENTRY(17, 1, bool, btnRight);
      BITFIELD_ENTRY(18, 1, bool, btnDown);
      BITFIELD_ENTRY(19, 1, bool, btnUp);
      BITFIELD_ENTRY(20, 1, bool, btnTriggerZ);
      BITFIELD_ENTRY(21, 1, bool, btnTriggerR);
      BITFIELD_ENTRY(22, 1, bool, btnTriggerL);
      BITFIELD_ENTRY(23, 1, bool, useOrigin);
      BITFIELD_ENTRY(24, 1, bool, btnA);
      BITFIELD_ENTRY(25, 1, bool, btnB);
      BITFIELD_ENTRY(26, 1, bool, btnX);
      BITFIELD_ENTRY(27, 1, bool, btnY);
      BITFIELD_ENTRY(28, 1, bool, btnPlus);
      BITFIELD_ENTRY(29, 1, bool, getOrigin);
      BITFIELD_ENTRY(31, 1, bool, error);
   BITFIELD_END

   BITFIELD(ControllerStatus1, uint32_t)
      BITFIELD_ENTRY(0, 8, uint8_t, analogB);
      BITFIELD_ENTRY(8, 8, uint8_t, analogA);
      BITFIELD_ENTRY(16, 8, uint8_t, stickRY);
      BITFIELD_ENTRY(24, 8, uint8_t, stickRX);
   BITFIELD_END

   struct ControllerRegisters
   {
      ControllerCommand command;
      ControllerStatus0 status0;
      ControllerStatus1 status1;
   };

   ControllerRegisters controllers[4];
   PollControl pollControl;
   DeviceStatus deviceStatus;
   ControllerError controllerError;
};

#pragma pack(pop)

uint16_t
OSReadRegister16(OSDeviceID device,
                 uint32_t id);

void
OSWriteRegister16(OSDeviceID device,
                  uint32_t id,
                  uint16_t value);

uint32_t
OSReadRegister32Ex(OSDeviceID device,
                   uint32_t id);

void
OSWriteRegister32Ex(OSDeviceID device,
                    uint32_t id,
                    uint32_t value);

/** @} */

} // namespace coreinit

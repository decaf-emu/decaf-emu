#pragma once
#include "ios_kernel_enum.h"
#include "ios_kernel_messagequeue.h"
#include "ios/ios_result.h"

#include <common/bitfield.h>

namespace ios::kernel
{

BITFIELD(AHBALL, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, Timer)
   BITFIELD_ENTRY(1, 1, bool, NandInterface)
   BITFIELD_ENTRY(2, 1, bool, AesEngine)
   BITFIELD_ENTRY(3, 1, bool, Sha1Engine)
   BITFIELD_ENTRY(4, 1, bool, UsbEhci)
   BITFIELD_ENTRY(5, 1, bool, UsbOhci0)
   BITFIELD_ENTRY(6, 1, bool, UsbOhci1)
   BITFIELD_ENTRY(7, 1, bool, SdHostController)
   BITFIELD_ENTRY(8, 1, bool, Wireless80211)

   BITFIELD_ENTRY(10, 1, bool, LatteGpioEspresso)
   BITFIELD_ENTRY(11, 1, bool, LatteGpioStarbuck)
   BITFIELD_ENTRY(12, 1, bool, SysProt)

   BITFIELD_ENTRY(17, 1, bool, PowerButton)
   BITFIELD_ENTRY(18, 1, bool, DriveInterface)

   BITFIELD_ENTRY(20, 1, bool, ExiRtc)

   BITFIELD_ENTRY(28, 1, bool, Sata)

   BITFIELD_ENTRY(30, 1, bool, IpcEspressoCompat)
   BITFIELD_ENTRY(31, 1, bool, IpcStarbuckCompat)
BITFIELD_END

BITFIELD(AHBLT, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, Unknown0)
   BITFIELD_ENTRY(1, 1, bool, Unknown1)
   BITFIELD_ENTRY(2, 1, bool, Unknown2)
   BITFIELD_ENTRY(3, 1, bool, Unknown3)
   BITFIELD_ENTRY(4, 1, bool, Drh)
   BITFIELD_ENTRY(5, 1, bool, Unknown5)
   BITFIELD_ENTRY(6, 1, bool, Unknown6)
   BITFIELD_ENTRY(7, 1, bool, Unknown7)
   BITFIELD_ENTRY(8, 1, bool, AesEngine)
   BITFIELD_ENTRY(9, 1, bool, Sha1Engine)
   BITFIELD_ENTRY(10, 1, bool, Unknown10)
   BITFIELD_ENTRY(11, 1, bool, Unknown11)
   BITFIELD_ENTRY(12, 1, bool, Unknown12)
   BITFIELD_ENTRY(13, 1, bool, I2CEspresso)
   BITFIELD_ENTRY(14, 1, bool, I2CStarbuck)

   BITFIELD_ENTRY(26, 1, bool, IpcEspressoCore2)
   BITFIELD_ENTRY(27, 1, bool, IpcStarbuckCore2)
   BITFIELD_ENTRY(28, 1, bool, IpcEspressoCore1)
   BITFIELD_ENTRY(29, 1, bool, IpcStarbuckCore1)
   BITFIELD_ENTRY(30, 1, bool, IpcEspressoCore0)
   BITFIELD_ENTRY(31, 1, bool, IpcStarbuckCore0)
BITFIELD_END

Error
IOS_HandleEvent(DeviceID id,
                MessageQueueID queue,
                Message message);

Error
IOS_ClearAndEnable(DeviceID id);

namespace internal
{

void
handleAhbInterrupts();

void
setInterruptAhbAll(AHBALL mask);

void
setInterruptAhbLt(AHBLT mask);

} // namespace internal

} // namespace ios::kernel

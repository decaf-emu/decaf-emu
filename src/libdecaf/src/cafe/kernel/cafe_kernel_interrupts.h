#pragma once
#include "cafe_kernel_context.h"
#include "cafe_kernel_processid.h"

namespace cafe::kernel
{

#include <common/enum_start.h>

ENUM_BEG(InterruptType, uint32_t)
   ENUM_VALUE(Error,          0)
   ENUM_VALUE(Dsp,            1)
   ENUM_VALUE(Gpu7,           2)
   ENUM_VALUE(GpiPpc,         3)
   ENUM_VALUE(PrimaryI2C,     4)
   ENUM_VALUE(DspAi,          5)
   ENUM_VALUE(DspAi2,         6)
   ENUM_VALUE(DspAcc,         7)
   ENUM_VALUE(DspDsp,         8)
   ENUM_VALUE(IpcPpc0,        9)
   ENUM_VALUE(IpcPpc1,        10)
   ENUM_VALUE(IpcPpc2,        11)
   ENUM_VALUE(Ahb,            12)
   ENUM_VALUE(Max,            13)
ENUM_END(InterruptType)

#include <common/enum_end.h>

using InterruptHandlerFn =
   void(*)(InterruptType type,
           virt_ptr<Context> interruptedContext);

InterruptHandlerFn
setUserModeInterruptHandler(InterruptType type,
                            InterruptHandlerFn callback);

namespace internal
{

void
dispatchExternalInterrupt(InterruptType type,
                          virt_ptr<Context> interruptContext);

void
setKernelInterruptHandler(InterruptType type,
                          InterruptHandlerFn handler);

} // namespace internal

} // namespace cafe::kernel

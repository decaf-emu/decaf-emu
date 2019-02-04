#pragma once
#include "tcl_enum.h"

#include <libcpu/be2_struct.h>
#include <libgpu/latte/latte_enum_cp.h>

namespace cafe::tcl
{

using TCLInterruptType = latte::CP_INT_SRC_ID;

struct TCLInterruptEntry
{
   be2_val<TCLInterruptType> interruptSourceID;
   be2_val<uint32_t> reservedWord1;
   be2_val<uint32_t> interruptSourceData;
   be2_val<uint32_t> reservedWord3;
};
CHECK_OFFSET(TCLInterruptEntry, 0x00, interruptSourceID);
CHECK_OFFSET(TCLInterruptEntry, 0x04, reservedWord1);
CHECK_OFFSET(TCLInterruptEntry, 0x08, interruptSourceData);
CHECK_OFFSET(TCLInterruptEntry, 0x0C, reservedWord3);
CHECK_SIZE(TCLInterruptEntry, 0x10);

using TCLInterruptHandlerFn = virt_func_ptr<
   void (virt_ptr<TCLInterruptEntry> interruptEntry, virt_ptr<void> userData)
>;

TCLStatus
TCLIHEnableInterrupt(TCLInterruptType type,
                     BOOL enable);

TCLStatus
TCLIHRegister(TCLInterruptType type,
              TCLInterruptHandlerFn callback,
              virt_ptr<void> userData);

TCLStatus
TCLIHUnregister(TCLInterruptType type,
                TCLInterruptHandlerFn callback,
                virt_ptr<void> userData);

TCLStatus
TCLGetInterruptCount(TCLInterruptType type,
                     BOOL resetCount,
                     virt_ptr<uint32_t> count);

namespace internal
{

void
initialiseInterruptHandler();

} // namespace internal

} // namespace cafe::tcl

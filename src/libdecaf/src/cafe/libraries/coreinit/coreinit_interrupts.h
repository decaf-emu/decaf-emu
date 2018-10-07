#pragma once
#include "coreinit_context.h"

#include "cafe/kernel/cafe_kernel_interrupts.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
* \defgroup coreinit_interrupts Interrupts
* \ingroup coreinit
* @{
*/

using OSInterruptType = kernel::InterruptType;
using OSUserInterruptHandler = virt_func_ptr<
   void (OSInterruptType type, virt_ptr<OSContext> interruptedContext)
>;

BOOL
OSEnableInterrupts();

BOOL
OSDisableInterrupts();

BOOL
OSRestoreInterrupts(BOOL enable);

BOOL
OSIsInterruptEnabled();

OSUserInterruptHandler
OSSetInterruptHandler(OSInterruptType type,
                      OSUserInterruptHandler handler);

void
OSClearAndEnableInterrupt(OSInterruptType type);

void
OSDisableInterrupt(OSInterruptType type);

namespace internal
{

void
initialiseIci();

} // namespace internal

/** @} */

} // namespace cafe::coreinit

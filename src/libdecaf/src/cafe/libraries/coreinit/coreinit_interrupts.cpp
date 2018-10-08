#include "coreinit.h"
#include "coreinit_context.h"
#include "coreinit_interrupts.h"
#include "coreinit_scheduler.h"

#include "cafe/cafe_ppc_interface_invoke.h"
#include "cafe/kernel/cafe_kernel_exception.h"
#include "cafe/kernel/cafe_kernel_interrupts.h"

#include <libcpu/cpu.h>

namespace cafe::coreinit
{

struct StaticInterruptsData
{
   be2_array<OSUserInterruptHandler, OSInterruptType::Max> registeredHandlers;
};

static virt_ptr<StaticInterruptsData>
sInterruptsData = nullptr;


/**
 * Enable interrupts on current core.
 *
 * \return Returns TRUE if interrupts were previously enabled, FALSE otherwise.
 */
BOOL
OSEnableInterrupts()
{
   return cpu::this_core::setInterruptMask(cpu::INTERRUPT_MASK) == cpu::INTERRUPT_MASK;
}


/**
 * Disable interrupts on current core.
 *
 * \return Returns TRUE if interrupts were previously enabled, FALSE otherwise.
 */
BOOL
OSDisableInterrupts()
{
   // We allow BreakpointException here so that the debugger can still trace through
   // OSDisableInterrupts calls.  This is not an issue only because internally we
   // only care about the scheduler lock which is only used internally.
   return cpu::this_core::setInterruptMask(cpu::DBGBREAK_INTERRUPT) == cpu::INTERRUPT_MASK;
}


/**
 * Sets if interrupts are enabled for current core.
 *
 * \return Returns TRUE if interrupts were previously enabled, FALSE otherwise.
 */
BOOL
OSRestoreInterrupts(BOOL enable)
{
   if (enable) {
      return OSEnableInterrupts();
   } else {
      return OSDisableInterrupts();
   }
}


/**
 * Check whether interrupts are enabled for current core.
 *
 * \return Returns TRUE if interrupts are enabled on current core.
 */
BOOL
OSIsInterruptEnabled()
{
   return cpu::this_core::interruptMask() == cpu::INTERRUPT_MASK;
}


static void
userInterruptHandler(OSInterruptType type,
                     virt_ptr<OSContext> interruptedContext,
                     virt_ptr<void> userData)
{
   auto userHandler = virt_func_cast<OSUserInterruptHandler>(virt_cast<virt_addr>(userData));
   if (userHandler) {
      internal::disableScheduler();
      cafe::invoke(cpu::this_core::state(),
                   userHandler,
                   type,
                   interruptedContext);
      internal::enableScheduler();
   }
}

OSUserInterruptHandler
OSSetInterruptHandler(OSInterruptType type,
                      OSUserInterruptHandler handler)
{
   auto previous = OSUserInterruptHandler { nullptr };
   if (type < OSInterruptType::Max) {
      previous = sInterruptsData->registeredHandlers[type];
      sInterruptsData->registeredHandlers[type] = handler;
   }

   kernel::setUserModeInterruptHandler(type,
                                       &userInterruptHandler,
                                       virt_cast<void *>(virt_func_cast<virt_addr>(handler)));
   return previous;
}


void
OSClearAndEnableInterrupt(OSInterruptType type)
{
   kernel::clearAndEnableInterrupt(type);
}


void
OSDisableInterrupt(OSInterruptType type)
{
   kernel::disableInterrupt(type);
}


namespace internal
{

static void
userModeIciCallback(kernel::ExceptionType type,
                    virt_ptr<kernel::Context> interruptedContext)
{
   lockScheduler();
   rescheduleSelfNoLock();
   unlockScheduler();
}

void
initialiseIci()
{
   kernel::setUserModeExceptionHandler(kernel::ExceptionType::ICI,
                                       userModeIciCallback);
}

} // namespace internal

void
Library::registerInterruptSymbols()
{
   RegisterFunctionExport(OSEnableInterrupts);
   RegisterFunctionExport(OSDisableInterrupts);
   RegisterFunctionExport(OSRestoreInterrupts);
   RegisterFunctionExport(OSIsInterruptEnabled);

   RegisterFunctionExportName("__OSSetInterruptHandler",
                              OSSetInterruptHandler);
   RegisterFunctionExportName("__OSClearAndEnableInterrupt",
                              OSClearAndEnableInterrupt);
   RegisterFunctionExportName("__OSDisableInterrupt",
                              OSDisableInterrupt);

   RegisterDataInternal(sInterruptsData);
}

} // namespace cafe::coreinit

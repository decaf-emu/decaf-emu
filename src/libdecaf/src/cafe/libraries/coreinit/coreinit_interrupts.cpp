#include "coreinit.h"

#include <libcpu/cpu.h>

namespace cafe::coreinit
{

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

void
Library::registerInterruptSymbols()
{
   RegisterFunctionExport(OSEnableInterrupts);
   RegisterFunctionExport(OSDisableInterrupts);
   RegisterFunctionExport(OSRestoreInterrupts);
   RegisterFunctionExport(OSIsInterruptEnabled);
}

} // namespace cafe::coreinit

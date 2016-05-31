#include "coreinit.h"
#include "coreinit_core.h"
#include "cpu/cpu.h"

namespace coreinit
{

/**
 * Enable interrupts on current core.
 *
 * \return Returns TRUE if interrupts were previously enabled, FALSE otherwise.
 */
BOOL
OSEnableInterrupts()
{
   return cpu::this_core::enableInterrupts() ? TRUE : FALSE;
}


/**
 * Disable interrupts on current core.
 *
 * \return Returns TRUE if interrupts were previously enabled, FALSE otherwise.
 */
BOOL
OSDisableInterrupts()
{
   return cpu::this_core::disableInterrupts() ? TRUE : FALSE;
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
   return cpu::this_core::isInterruptsEnabled() ? TRUE : FALSE;
}

void
Module::registerInterruptFunctions()
{
   RegisterKernelFunction(OSEnableInterrupts);
   RegisterKernelFunction(OSDisableInterrupts);
   RegisterKernelFunction(OSRestoreInterrupts);
   RegisterKernelFunction(OSIsInterruptEnabled);
}

} // namespace coreinit

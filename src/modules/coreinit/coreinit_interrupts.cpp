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
   return cpu::this_core::setInterruptMask(0) == cpu::INTERRUPT_MASK;
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
Module::registerInterruptFunctions()
{
   RegisterKernelFunction(OSEnableInterrupts);
   RegisterKernelFunction(OSDisableInterrupts);
   RegisterKernelFunction(OSRestoreInterrupts);
   RegisterKernelFunction(OSIsInterruptEnabled);
}

} // namespace coreinit

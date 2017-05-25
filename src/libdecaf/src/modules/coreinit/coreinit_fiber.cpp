#include "coreinit.h"
#include "coreinit_fiber.h"
#include "coreinit_thread.h"
#include "ppcutils/wfunc_call.h"

#include <cstdint>
#include <libcpu/cpu.h>

namespace coreinit
{


/**
 * Switch to fiber.
 *
 * Calls fn() with a r1=userStack
 */
void
OSSwitchFiber(OSFiberEntryFn fn,
              uint32_t userStack)
{
   auto state = cpu::this_core::state();
   auto oldStack = state->gpr[1];

   // Set new stack
   internal::setUserStackPointer(userStack);
   state->gpr[1] = userStack;

   // Call fiber function
   fn();

   // Restore old stack
   state->gpr[1] = oldStack;
   internal::removeUserStackPointer(oldStack);
}


/**
 * Switch to fiber with arguments.
 *
 * Calls fn(r3, r4, r5, r6) with a r1=userStack
 */
void
OSSwitchFiberEx(uint32_t r3,
                uint32_t r4,
                uint32_t r5,
                uint32_t r6,
                OSFiberExEntryFn fn,
                uint32_t userStack)
{
   auto state = cpu::this_core::state();
   auto oldStack = state->gpr[1];

   // Set new stack
   internal::setUserStackPointer(userStack);
   state->gpr[1] = userStack;

   // Call fiber function
   fn(r3, r4, r5, r6);

   // Restore old stack
   state->gpr[1] = oldStack;
   internal::removeUserStackPointer(oldStack);
}

void
Module::registerFiberFunctions()
{
   RegisterKernelFunction(OSSwitchFiber);
   RegisterKernelFunction(OSSwitchFiberEx);
}

} // namespace coreinit

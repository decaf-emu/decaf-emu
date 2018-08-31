#include "coreinit.h"
#include "coreinit_fiber.h"
#include "coreinit_thread.h"
#include "cafe/cafe_ppc_interface_invoke.h"
#include <libcpu/cpu.h>

namespace cafe::coreinit
{

/**
 * Switch to fiber.
 */
uint32_t
OSSwitchFiber(OSFiberEntryFn entry,
              virt_addr userStack)
{

   auto state = cpu::this_core::state();
   auto oldStack = virt_addr { state->gpr[1] };

   // Set new stack
   internal::setUserStackPointer(virt_cast<uint32_t *>(userStack));
   state->gpr[1] = static_cast<uint32_t>(userStack);

   // Call fiber function
   auto result = cafe::invoke(cpu::this_core::state(), entry);

   // Restore old stack
   state->gpr[1] = static_cast<uint32_t>(oldStack);
   internal::removeUserStackPointer(virt_cast<uint32_t *>(oldStack));

   return result;
}

/**
 * Switch to fiber with 4 arguments.
 */
uint32_t
OSSwitchFiberEx(uint32_t arg1,
                uint32_t arg2,
                uint32_t arg3,
                uint32_t arg4,
                OSFiberExEntryFn entry,
                virt_addr userStack)
{
   auto state = cpu::this_core::state();
   auto oldStack = virt_addr { state->gpr[1] };

   // Set new stack
   internal::setUserStackPointer(virt_cast<uint32_t *>(userStack));
   state->gpr[1] = static_cast<uint32_t>(userStack);

   // Call fiber function
   auto result = cafe::invoke(cpu::this_core::state(), entry,
                              arg1, arg2, arg3, arg4);

   // Restore old stack
   state->gpr[1] = static_cast<uint32_t>(oldStack);
   internal::removeUserStackPointer(virt_cast<uint32_t *>(oldStack));

   return result;
}

void
Library::registerFiberSymbols()
{
   RegisterFunctionExport(OSSwitchFiber);
   RegisterFunctionExport(OSSwitchFiberEx);
}

} // namespace cafe::coreinit

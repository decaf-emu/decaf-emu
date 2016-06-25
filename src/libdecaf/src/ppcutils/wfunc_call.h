#pragma once
#include "ppcinvoke.h"
#include "libcpu/cpu.h"
#include "wfunc_ptr.h"

template<typename ReturnType, typename... Args>
ReturnType wfunc_ptr<ReturnType, Args...>::operator()(Args... args)
{
   auto core = cpu::this_core::state();

   // TODO: ppctypes should use CoreRegs& not Core*

   // Push args
   ppctypes::applyArguments(core, args...);

   // For debugging, lets grab a copy of the SP
   auto origStackPtr = core->gpr[1];

   // Allocate callee backchain and lr space.
   core->gpr[1] -= 2 * 4;

   // Save state
   auto nia = core->nia;

   // Set state to our function to call
   core->cia = 0;
   core->nia = address;

   // Start executing!
   cpu::this_core::executeSub();

   // Grab the most recent core state as it may have changed.
   core = cpu::this_core::state();

   // Restore state
   core->nia = nia;

   // Restore callee args stack space
   core->gpr[1] += 2 * 4;

   // Lets verify we did not corrupt the SP
   emuassert(core->gpr[1] == origStackPtr);

   // Return the result
   return ppctypes::getResult<ReturnType>(core);
}

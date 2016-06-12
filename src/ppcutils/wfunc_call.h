#pragma once
#include "ppcinvoke.h"
#include "cpu/cpu.h"
#include "wfunc_ptr.h"

template<typename ReturnType, typename... Args>
ReturnType wfunc_ptr<ReturnType, Args...>::operator()(Args... args)
{
   auto core = cpu::this_core::state();

   // Push args
   ppctypes::applyArguments(core, args...);

   // Allocate callee backchain and lr space.
   core->gpr[1] -= 2 * 4;

   // Save state
   auto nia = core->nia;

   // Set state
   core->cia = 0;
   core->nia = address;
   cpu::this_core::executeSub();

   // Restore state
   core->nia = nia;

   // Restore callee args stack space
   core->gpr[1] += 2 * 4;

   // Return the result
   return ppctypes::getResult<ReturnType>(core);
}

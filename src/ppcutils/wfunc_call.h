#pragma once
#include "ppcinvoke.h"
#include "cpu/cpu.h"
#include "wfunc_ptr.h"

template<typename ReturnType, typename... Args>
ReturnType wfunc_ptr<ReturnType, Args...>::operator()(Args... args)
{
   auto core = cpu::this_core::state();

   // Push args
   auto registersUsed = static_cast<uint32_t>(ppctypes::applyArguments(core, args...));

   // Allocate callee args stack space
   core->gpr[1] -= registersUsed * 4;

   // Save state
   auto nia = core->nia;

   // Set state
   core->cia = 0;
   core->nia = address;
   cpu::this_core::execute_sub();

   // Restore state
   core->nia = nia;

   // Restore callee args stack space
   core->gpr[1] += registersUsed * 4;

   // Return the result
   return ppctypes::getResult<ReturnType>(core);
}

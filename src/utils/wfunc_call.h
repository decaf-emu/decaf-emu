#pragma once
#include "cpu/cpu.h"
#include "wfunc_ptr.h"

template<typename ReturnType, typename... Args>
ReturnType wfunc_ptr<ReturnType, Args...>::operator()(Args... args)
{
   auto core = cpu::this_core::state();

   // Push args
   ppctypes::applyArguments(core, args...);

   // Save state
   auto nia = core->nia;

   // Set state
   core->cia = 0;
   core->nia = address;
   cpu::this_core::execute_sub();

   // Restore state
   core->nia = nia;

   // Return the result
   return ppctypes::getResult<ReturnType>(core);
}

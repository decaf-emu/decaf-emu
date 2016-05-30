#pragma once
#include "cpu/cpu.h"
#include "wfunc_ptr.h"

template<typename ReturnType, typename... Args>
ReturnType wfunc_ptr<ReturnType, Args...>::operator()(Args... args)
{
   ThreadState *state = cpu::get_current_core()->state;

   // Push args
   ppctypes::applyArguments(state, args...);

   // Save state
   auto nia = state->nia;

   // Set state
   state->cia = 0;
   state->nia = address;
   cpu::core_execute_sub();

   // Restore state
   state->nia = nia;

   // Return the result
   return ppctypes::getResult<ReturnType>(state);
}

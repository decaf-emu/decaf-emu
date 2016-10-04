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

   // Write LR to the previous backchain address
   auto backchain = mem::read<uint32_t>(origStackPtr);

   if (backchain) {
      // This might be the very first call on a core...
      mem::write(backchain + 4, core->lr);
   }

   // Save state
   auto cia = core->cia;
   auto nia = core->nia;

   // Set state to our function to call
   core->cia = 0;
   core->nia = address;

   // Start executing!
   cpu::this_core::executeSub();

   // Grab the most recent core state as it may have changed.
   core = cpu::this_core::state();

   // Restore state
   core->cia = cia;
   core->nia = nia;

   // Lets verify we did not corrupt the SP
   decaf_check(core->gpr[1] == origStackPtr);

   // Return the result
   return ppctypes::getResult<ReturnType>(core);
}

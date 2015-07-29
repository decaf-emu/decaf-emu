#pragma once
#include <cstdint>
#include <ostream>
#include "p32.h"
#include "ppc.h"

#pragma pack(push, 1)

template<typename ReturnType, typename... Args>
struct wfunc_ptr
{
   wfunc_ptr() :
      address(0)
   {
   }

   wfunc_ptr(std::nullptr_t) :
      address(0)
   {
   }

   wfunc_ptr(int addr) :
      address(addr)
   {
   }

   wfunc_ptr(uint32_t addr) :
      address(addr)
   {
   }

   wfunc_ptr(p32<void> addr) :
      address(addr)
   {
   }

   operator uint32_t() const
   {
      return address;
   }

   uint32_t address;

   ReturnType call(ThreadState *state, Args... args);

   ReturnType operator()(Args... args) {
      ThreadState *state = &gProcessor.getCurrentFiber()->state;
      return call(state, args...);
   }

};

#pragma pack(pop)


template<typename ReturnType, typename... Args>
static inline std::ostream&
operator<<(std::ostream& os, const wfunc_ptr<ReturnType, Args...>& val)
{
   return os << static_cast<uint32_t>(val);
}

// Late include of ppcinvoke due to circular reference of wfunc_ptr inside arg_converter.
#include "ppcinvoke.h"
template<typename ReturnType, typename... Args>
ReturnType wfunc_ptr<ReturnType, Args...>::call(ThreadState *state, Args... args) {
   // Push args
   ppctypes::applyArguments(state, args...);

   // Save LR to stack
   state->gpr[1] -= 4;
   gMemory.write(state->gpr[1], state->lr);

   // Save NIA to LR
   state->lr = state->nia;

   // Update NIA to Target
   state->cia = 0;
   state->nia = addr;

   gInterpreter.execute(state);

   // Restore NIA from LR
   state->nia = state->lr;

   // Restore LR from Stack
   state->lr = gMemory.read<uint32_t>(state->gpr[1]);
   state->gpr[1] += 4;

   // Return the result
   return ppctypes::getResult<ReturnType>(state);
}

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
#include "interpreter.h"

template<typename ReturnType, typename... Args>
ReturnType wfunc_ptr<ReturnType, Args...>::call(ThreadState *state, Args... args) {
   // Push args
   ppctypes::applyArguments(state, args...);

   // Save state
   auto lr = state->lr;
   auto nia = state->nia;

   // Set state
   state->cia = 0;
   state->nia = address;
   state->lr = CALLBACK_ADDR;

   gInterpreter.execute(state);

   // Restore state
   state->lr = lr;
   state->nia = nia;

   // Return the result
   return ppctypes::getResult<ReturnType>(state);
}

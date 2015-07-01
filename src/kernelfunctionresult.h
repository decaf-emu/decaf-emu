#pragma once
#include "ppc.h"
#include "systemtypes.h"

// TODO: Find out how floats are returned

namespace kernel
{

namespace functions
{

template<typename Type>
struct result_converter_t;

// p32<Type>
template<typename Type>
struct result_converter_t<p32<Type>>
{
   static inline void
   update(ThreadState *state, p32<Type> v)
   {
      state->gpr[3] = static_cast<uint32_t>(v);
   }
};

// Type*
template<typename Type>
struct result_converter_t<Type *>
{
   static inline void
   update(ThreadState *state, Type *ptr)
   {
      state->gpr[3] = gMemory.untranslate(ptr);
   }
};

// int64_t
template<>
struct result_converter_t<int64_t>
{
   static inline void
   update(ThreadState *state, int64_t v)
   {
      state->gpr[3] = static_cast<uint32_t>(v >> 32);
      state->gpr[4] = static_cast<uint32_t>(v & 0xffffffff);
   }
};

// uint64_t
template<>
struct result_converter_t<uint64_t>
{
   static inline void
   update(ThreadState *state, uint64_t v)
   {
      state->gpr[3] = static_cast<uint32_t>(v >> 32);
      state->gpr[4] = static_cast<uint32_t>(v & 0xffffffff);
   }
};

// Generic Type
template<typename Type>
struct result_converter_t
{
   static inline void
   update(ThreadState *state, Type v)
   {
      state->gpr[3] = static_cast<uint32_t>(v);
   }
};

// Copy return result to registers
template<typename Type>
static inline void
setResult(ThreadState *state, Type v)
{
   result_converter_t<Type>::update(state, v);
}

}

}

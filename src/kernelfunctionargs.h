#pragma once
#include "log.h"
#include "p32.h"
#include "ppc.h"

namespace kernel
{

namespace functions
{

template<typename Type>
struct arg_converter_t;

// p32<Type>
template<typename Type>
struct arg_converter_t<p32<Type>>
{
   static inline p32<Type>
   convert(ThreadState *state, size_t &r, size_t &f)
   {
      return make_p32<Type>(state->gpr[r++]);
   }
};

// Type *
template<typename Type>
struct arg_converter_t<Type *>
{
   static inline Type *
   convert(ThreadState *state, size_t &r, size_t &f)
   {
      auto value = state->gpr[r++];

      if (value == 0) {
         return nullptr;
      } else {
         return reinterpret_cast<Type*>(gMemory.translate(value));
      }
   }
};

// float
template<>
struct arg_converter_t<float>
{
   static inline float
   convert(ThreadState *state, size_t &r, size_t &f)
   {
      return state->fpr[f++].paired0;
   }
};

// double
template<>
struct arg_converter_t<double>
{
   static inline double
   convert(ThreadState *state, size_t &r, size_t &f)
   {
      return state->fpr[f++].value;
   }
};

// Generic
template<typename Type>
struct arg_converter_t
{
   static inline Type
   convert(ThreadState *state, size_t &r, size_t &f)
   {
      return static_cast<Type>(state->gpr[r++]);
   }
};

// Grab an argument from registers for function
template<typename Type>
static inline Type
convertArgument(ThreadState *state, size_t &r, size_t &f)
{
   return arg_converter_t<Type>::convert(state, r, f);
}

}

}

#pragma once
#include "ppctypes.h"

// TODO: Find out how floats are returned

namespace ppctypes
{

template <bool IsFloat, int PpcSize, typename Type>
struct result_converter_t;

template<typename Type>
struct result_converter_t<false, 1, Type> {
   static inline void set(ThreadState *state, Type v) {
      ppctype_converter_t<Type>::to_ppc(v, state->gpr[3]);
   }

   static inline Type get(ThreadState *state) {
      return ppctype_converter_t<Type>::from_ppc(state->gpr[3]);
   }
};

template<typename Type>
struct result_converter_t<false, 2, Type> {
   static inline void set(ThreadState *state, Type v) {
      ppctype_converter_t<Type>::to_ppc(v, state->gpr[3], state->gpr[4]);
   }

   static inline Type get(ThreadState *state) {
      return ppctype_converter_t<Type>::from_ppc(state->gpr[3], state->gpr[4]);
   }
};

// Copy return result to registers
template<typename Type>
static inline void
setResult(ThreadState *state, Type v)
{
   return result_converter_t<
      ppctype_converter_t<Type>::is_float,
      ppctype_converter_t<Type>::ppc_size,
      Type>::set(state, v);
}

template<typename Type>
static inline Type
getResult(ThreadState *state)
{
   return result_converter_t<
      ppctype_converter_t<Type>::is_float,
      ppctype_converter_t<Type>::ppc_size,
      Type>::get(state);
}

}

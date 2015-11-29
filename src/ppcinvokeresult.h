#pragma once
#include "ppctypes.h"

// TODO: Find out how floats are returned

namespace ppctypes
{

template <PpcType PpcTypeId, typename Type>
struct result_converter_t;

template<typename Type>
struct result_converter_t<PpcType::WORD, Type>
{
   static inline void set(ThreadState *state, Type v)
   {
      ppctype_converter_t<Type>::to_ppc(v, state->gpr[3]);
   }

   static inline Type get(ThreadState *state)
   {
      return ppctype_converter_t<Type>::from_ppc(state->gpr[3]);
   }
};

template<typename Type>
struct result_converter_t<PpcType::DWORD, Type>
{
   static inline void set(ThreadState *state, Type v)
   {
      ppctype_converter_t<Type>::to_ppc(v, state->gpr[3], state->gpr[4]);
   }

   static inline Type get(ThreadState *state)
   {
      return ppctype_converter_t<Type>::from_ppc(state->gpr[3], state->gpr[4]);
   }
};

// Copy return result to registers
template<typename Type>
inline void
setResult(ThreadState *state, Type v)
{
   result_converter_t<ppctype_converter_t<Type>::ppc_type, Type>::set(state, v);
}

template<typename Type>
inline Type
getResult(ThreadState *state)
{
   return result_converter_t<ppctype_converter_t<Type>::ppc_type, Type>::get(state);
}

template <>
inline void
getResult<void>(ThreadState *state)
{
}

} // namespace ppctypes

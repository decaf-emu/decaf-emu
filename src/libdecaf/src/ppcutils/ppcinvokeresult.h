#pragma once
#include "ppctypeconv.h"

namespace ppctypes
{

template <PpcType PpcTypeId, typename Type>
struct result_converter_t;

template<typename Type>
struct result_converter_t<PpcType::WORD, Type>
{
   static inline void set(cpu::Core *state, Type v)
   {
      ppctype_converter_t<Type>::to_ppc(v, state->gpr[3]);
   }

   static inline Type get(cpu::Core *state)
   {
      return ppctype_converter_t<Type>::from_ppc(state->gpr[3]);
   }
};

template<typename Type>
struct result_converter_t<PpcType::DWORD, Type>
{
   static inline void set(cpu::Core *state, Type v)
   {
      ppctype_converter_t<Type>::to_ppc(v, state->gpr[3], state->gpr[4]);
   }

   static inline Type get(cpu::Core *state)
   {
      return ppctype_converter_t<Type>::from_ppc(state->gpr[3], state->gpr[4]);
   }
};

template<typename Type>
struct result_converter_t<PpcType::FLOAT, Type>
{
   static inline void set(cpu::Core *state, Type v)
   {
      ppctype_converter_t<Type>::to_ppc(v, state->fpr[1].value);
   }

   static inline Type get(cpu::Core *state)
   {
      return ppctype_converter_t<Type>::from_ppc(state->fpr[1].value);
   }
};

template<typename Type>
struct result_converter_t<PpcType::DOUBLE, Type>
{
   static inline void set(cpu::Core *state, Type v)
   {
      ppctype_converter_t<Type>::to_ppc(v, state->fpr[1].value);
   }

   static inline Type get(cpu::Core *state)
   {
      return ppctype_converter_t<Type>::from_ppc(state->fpr[1].value);
   }
};

// Copy return result to registers
template<typename Type>
inline void
setResult(cpu::Core *state, Type v)
{
   result_converter_t<ppctype_converter_t<Type>::ppc_type, Type>::set(state, v);
}

template<typename Type>
inline Type
getResult(cpu::Core *state)
{
   return result_converter_t<ppctype_converter_t<Type>::ppc_type, Type>::get(state);
}

template <>
inline void
getResult<void>(cpu::Core *state)
{
}

} // namespace ppctypes

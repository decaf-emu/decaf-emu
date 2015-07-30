#pragma once
#include "ppctypes.h"

namespace ppctypes
{

template <PpcType PpcTypeId, typename Type>
struct arg_converter_t;

template<typename Type>
struct arg_converter_t<PpcType::WORD, Type> {
   static inline Type get(ThreadState *state, size_t &r, size_t &f)
   {
      auto& x = state->gpr[r++];
      return ppctype_converter_t<Type>::from_ppc(x);
   }

   static inline void set(ThreadState *state, size_t &r, size_t &f, Type v)
   {
      auto& x = state->gpr[r++];
      ppctype_converter_t<Type>::to_ppc(v, x);
   }
};

template<typename Type>
struct arg_converter_t<PpcType::DWORD, Type> {
   static inline Type get(ThreadState *state, size_t &r, size_t &f)
   {
      auto& x = state->gpr[r++];
      auto& y = state->gpr[r++];
      return ppctype_converter_t<Type>::from_ppc(x, y);
   }

   static inline void set(ThreadState *state, size_t &r, size_t &f, Type v)
   {
      auto& x = state->gpr[r++];
      auto& y = state->gpr[r++];
      ppctype_converter_t<Type>::to_ppc(v, x, y);
   }
};

template<typename Type>
struct arg_converter_t<PpcType::PAIRED0, Type> {
   static inline Type get(ThreadState *state, size_t &r, size_t &f)
   {
      auto& x = state->fpr[f++].paired0;
      return ppctype_converter_t<Type>::from_ppc(x);
   }

   static inline void set(ThreadState *state, size_t &r, size_t &f, Type v)
   {
      auto& x = state->fpr[f++].paired0;
      ppctype_converter_t<Type>::to_ppc(v, x);
   }
};

template<typename Type>
struct arg_converter_t<PpcType::DOUBLE, Type> {
   static inline Type get(ThreadState *state, size_t &r, size_t &f)
   {
      auto& x = state->fpr[f++].value;
      return ppctype_converter_t<Type>::from_ppc(x);
   }

   static inline void set(ThreadState *state, size_t &r, size_t &f, Type v)
   {
      auto& x = state->fpr[f++].value;
      ppctype_converter_t<Type>::to_ppc(v, x);
   }
};

template<typename Type>
static inline void
setArgument(ThreadState *state, size_t &r, size_t &f, Type v)
{
   return arg_converter_t<
      ppctype_converter_t<Type>::ppc_type,
      Type>::set(state, r, f, v);
}

// Grab an argument from registers for function
template<typename Type>
static inline Type
getArgument(ThreadState *state, size_t &r, size_t &f)
{
   return arg_converter_t<
      ppctype_converter_t<Type>::ppc_type,
      Type>::get(state, r, f);
}

}

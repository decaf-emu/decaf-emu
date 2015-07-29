#pragma once
#include <assert.h>
#include <type_traits>
#include "log.h"
#include "p32.h"
#include "wfunc_ptr.h"
#include "ppc.h"

namespace ppctypes
{

template<typename Type>
struct ppctype_converter_t;

// p32<Type>
template<typename Type>
struct ppctype_converter_t<p32<Type>>
{
   static const bool is_float = false;
   static const int ppc_size = 1;

   static inline void to_ppc(p32<Type> v, uint32_t& out) {
      out = static_cast<uint32_t>(v);
   }

   static inline p32<Type> from_ppc(uint32_t in) {
      return make_p32<Type>(in);
   }
};

// Type*
template<typename Type>
struct ppctype_converter_t<Type *>
{
   static const bool is_float = false;
   static const int ppc_size = 1;

   static inline void to_ppc(Type *ptr, uint32_t& out)
   {
      out = gMemory.untranslate(ptr);
   }

   static inline Type * from_ppc(uint32_t in) {
      if (in == 0) {
         return nullptr;
      } else {
         return reinterpret_cast<Type*>(gMemory.translate(in));
      }
   }
};

// int64_t
template<>
struct ppctype_converter_t<int64_t>
{
   static const bool is_float = false;
   static const int ppc_size = 2;

   static inline void to_ppc(int64_t v, uint32_t& out1, uint32_t& out2)
   {
      out1 = static_cast<uint32_t>(v >> 32);
      out2 = static_cast<uint32_t>(v & 0xffffffff);
   }

   static inline int64_t from_ppc(uint32_t in1, uint32_t in2) {
      auto x = static_cast<uint64_t>(in1);
      auto y = static_cast<uint64_t>(in2);
      return static_cast<int64_t>((x << 32) | y);
   }
};

// uint64_t
template<>
struct ppctype_converter_t<uint64_t>
{
   static const bool is_float = false;
   static const int ppc_size = 2;

   static inline void to_ppc(uint64_t v, uint32_t& out1, uint32_t& out2) {
      out1 = static_cast<uint32_t>(v >> 32);
      out2 = static_cast<uint32_t>(v & 0xffffffff);
   }

   static inline uint64_t from_ppc(uint32_t in1, uint32_t in2) {
      auto x = static_cast<uint64_t>(in1);
      auto y = static_cast<uint64_t>(in2);
      return (x << 32) | y;
   }
};

// bool
template<>
struct ppctype_converter_t<bool>
{
   static const bool is_float = false;
   static const int ppc_size = 1;

   static inline void to_ppc(bool v, uint32_t& out) {
      out = v ? 1 : 0;
   }

   static inline bool from_ppc(uint32_t in) {
      return in != 0;
   }
};

// wfunc_ptr<...>
template<typename ReturnType, typename... Args>
struct ppctype_converter_t<wfunc_ptr<ReturnType, Args...>>
{
   static const bool is_float = false;
   static const int ppc_size = 1;

   typedef wfunc_ptr<ReturnType, Args...> Type;
   
   static inline void to_ppc(const Type& v, uint32_t& out) {
      out = v.address;
   }

   static inline Type from_ppc(uint32_t in) {
      return Type(in);
   }
};

// float
template<>
struct ppctype_converter_t<float>
{
   static const bool is_float = true;
   static const int ppc_size = 1;

   static inline void to_ppc(float v, float& out) {
      out = v;
   }

   static inline float from_ppc(float in) {
      return in;
   }
};

// double
template<>
struct ppctype_converter_t<double>
{
   static const bool is_float = true;
   static const int ppc_size = 2;

   static inline void to_ppc(double v, double& out) {
      out = v;
   }

   static inline double from_ppc(double in) {
      return in;
   }
};

// Generic Type
template<typename Type>
struct ppctype_converter_t
{
   static_assert(
      std::is_integral<Type>::value || std::is_enum<Type>::value,
      "cannot do ppctype conversion against non-integer non-enum");
   static_assert(sizeof(Type) <= 4,
      "cannot do ppctype conversion for non-sizeof 4 type");

   static const bool is_float = false;
   static const int ppc_size = 1;

   static inline void to_ppc(Type v, uint32_t& out) {
      out = static_cast<uint32_t>(v);
   }

   static inline Type from_ppc(uint32_t in) {
      return static_cast<Type>(in);
   }
};

}

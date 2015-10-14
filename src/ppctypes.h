#pragma once
#include <assert.h>
#include <type_traits>
#include "ppc.h"
#include "memory_translate.h"

namespace ppctypes
{

enum class PpcType
{
   WORD,
   DWORD,
   FLOAT,
   DOUBLE
};

template<typename Type>
struct ppctype_converter_t;

// Type*
template<typename Type>
struct ppctype_converter_t<Type *>
{
   static const PpcType ppc_type = PpcType::WORD;

   static inline void to_ppc(Type *ptr, uint32_t& out)
   {
      out = memory_untranslate(ptr);
   }

   static inline Type * from_ppc(uint32_t in)
   {
      if (in == 0) {
         return nullptr;
      } else {
         return reinterpret_cast<Type*>(memory_translate(in));
      }
   }
};

// int64_t
template<>
struct ppctype_converter_t<int64_t>
{
   static const PpcType ppc_type = PpcType::DWORD;

   static inline void to_ppc(int64_t v, uint32_t& out1, uint32_t& out2)
   {
      auto uv = static_cast<uint64_t>(v);
      out1 = static_cast<uint32_t>(uv >> 32);
      out2 = static_cast<uint32_t>(uv & 0xffffffff);
   }

   static inline int64_t from_ppc(uint32_t in1, uint32_t in2)
   {
      auto x = static_cast<uint64_t>(in1);
      auto y = static_cast<uint64_t>(in2);
      return static_cast<int64_t>((x << 32) | y);
   }
};

// uint64_t
template<>
struct ppctype_converter_t<uint64_t>
{
   static const PpcType ppc_type = PpcType::DWORD;

   static inline void to_ppc(uint64_t v, uint32_t& out1, uint32_t& out2)
   {
      out1 = static_cast<uint32_t>(v >> 32);
      out2 = static_cast<uint32_t>(v & 0xffffffff);
   }

   static inline uint64_t from_ppc(uint32_t in1, uint32_t in2)
   {
      auto x = static_cast<uint64_t>(in1);
      auto y = static_cast<uint64_t>(in2);
      return (x << 32) | y;
   }
};

// bool
template<>
struct ppctype_converter_t<bool>
{
   static const PpcType ppc_type = PpcType::WORD;

   static inline void to_ppc(bool v, uint32_t& out)
   {
      out = v ? 1 : 0;
   }

   static inline bool from_ppc(uint32_t in)
   {
      return in != 0;
   }
};

// float
template<>
struct ppctype_converter_t<float>
{
   static const PpcType ppc_type = PpcType::FLOAT;

   static inline void to_ppc(float v, double& out)
   {
      out = static_cast<double>(v);
   }

   static inline float from_ppc(double in)
   {
      return static_cast<float>(in);
   }
};

// double
template<>
struct ppctype_converter_t<double>
{
   static const PpcType ppc_type = PpcType::DOUBLE;

   static inline void to_ppc(double v, double& out)
   {
      out = v;
   }

   static inline double from_ppc(double in)
   {
      return in;
   }
};

// Generic Type
template<typename Type>
struct ppctype_converter_t
{
   static_assert(std::is_integral<Type>::value || std::is_enum<Type>::value,
                 "Cannot do ppctype conversion against non-integer non-enum");
   static_assert(sizeof(Type) <= 4,
                 "Cannot do ppctype conversion for sizeof>4 type");

   static const PpcType ppc_type = PpcType::WORD;

   static inline void to_ppc(Type v, uint32_t& out)
   {
      out = static_cast<uint32_t>(v);
   }

   static inline Type from_ppc(uint32_t in)
   {
      return static_cast<Type>(in);
   }
};

}

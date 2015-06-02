#pragma once
#include <cstdint>
#include "memory.h"

#ifndef BOOL
typedef int BOOL;
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#pragma pack(push, 1)

using p32_raw = uint32_t;

template<typename Type>
struct p32
{
   p32 &operator=(Type *other)
   {
      value = reinterpret_cast<uint32_t>(other);
      return *this;
   }

   template<typename Other>
   p32 &operator=(p32<Other> other)
   {
      value = other.value;
      return *this;
   }

   Type *operator->() const
   {
      return reinterpret_cast<Type*>(gMemory.translate(value));
   }

   operator Type *() const
   {
      return reinterpret_cast<Type*>(gMemory.translate(value));
   }

   uint32_t value;
};

template<typename Type>
inline p32<Type>
make_p32(Type *src)
{
   p32<Type> result;
   result.value = src ? gMemory.untranslate(src) : 0;
   return result;
}

template<typename Type>
inline p32<Type>
make_p32(uint32_t vaddr)
{
   p32<Type> result;
   result.value = vaddr;
   return result;
}

template<typename Type>
inline Type *
p32_direct(p32<Type> ptr)
{
   return (Type*)ptr;
}

template<typename Type>
struct be_val
{
   template<typename OtherType>
   be_val &operator=(OtherType other)
   {
      value = byte_swap(other);
      return *this;
   }

   operator Type() const
   {
      return byte_swap(value);
   }

   Type value;
};

template<typename Type>
using be_ptr = be_val<p32<Type>>;

#pragma pack(pop)

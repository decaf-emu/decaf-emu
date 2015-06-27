#pragma once
#include <cstdint>
#include "memory.h"
#include "systemobject.h"

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
#include "be_types.h"
/*
template<typename Type>
struct p32
{
   p32()
   {
   }

   explicit p32(std::nullptr_t) :
      address(0)
   {
   }

   template<typename Other>
   p32(p32<Other> &other) :
      address(other.address)
   {
   }

   p32 &operator=(Type *other)
   {
      address = static_cast<uint32_t>(reinterpret_cast<size_t>(other));
      return *this;
   }

   template<typename Other>
   p32 &operator=(p32<Other> other)
   {
      address = other.address;
      return *this;
   }

   Type *operator->() const
   {
      return reinterpret_cast<Type*>(gMemory.translate(address));
   }

   operator Type *() const
   {
      return reinterpret_cast<Type*>(gMemory.translate(address));
   }

   bool operator==(const p32<Type> &other) const
   {
      return address == other.address;
   }

   explicit operator uint32_t()
   {
      return address;
   }

   uint32_t address;
};

template<typename Type>
inline p32<Type>
make_p32(const Type *src)
{
   p32<Type> result;
   result.address = src ? gMemory.untranslate(src) : 0;
   return result;
}

template<typename Type>
inline p32<Type>
make_p32(uint32_t vaddr)
{
   p32<Type> result;
   result.address = vaddr;
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
      value = static_cast<Type>(byte_swap(other));
      return *this;
   }

   operator Type() const
   {
      return byte_swap(value);
   }

   bool operator==(const be_val<Type> &other) const
   {
      return value == other.value;
   }

   bool operator==(const Type &other) const
   {
      return byte_swap(value) == other;
   }

   bool operator==(std::nullptr_t) const
   {
      return value == 0;
   }

   be_val<Type> &operator++()
   {
      ++value;
      return *this;
   }

   be_val<Type> &operator--()
   {
      --value;
      return *this;
   }

   Type value;
};

template<typename Type>
using be_ptr = be_val<p32<Type>>;
*/

template<typename ReturnType, typename... Args>
struct wfunc_ptr
{
   wfunc_ptr() :
      address(0)
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
};

#pragma pack(pop)

// Ensure our structs are correct size & offsets to match WiiU
#define CHECK_SIZE(Type, Size) \
   static_assert(sizeof(Type) == Size, \
                 #Type " must be " #Size " bytes");

#define CHECK_OFFSET(Type, Offset, Field) \
   static_assert((unsigned long long)&((Type*)0)->Field == Offset, \
                 #Type "::" #Field " must be at offset " #Offset);

// Workaround weird macro concat ## behaviour
#define PP_CAT(a, b) PP_CAT_I(a, b)
#define PP_CAT_I(a, b) PP_CAT_II(~, a ## b)
#define PP_CAT_II(p, res) res

// Allow us to easily add UNKNOWN / PADDING bytes into our structs,
// generates unique variable names using __COUNTER__
#define UNKNOWN(Size) char PP_CAT(__unk, __COUNTER__) [Size];
#define PADDING(Size) UNKNOWN(Size)

#define UNKNOWN_ARGS void

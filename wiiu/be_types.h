#pragma once
#include "bitutils.h"
#include <type_traits>

using addr_t = uint32_t;

template<typename Type>
class be_val
{
public:
   static_assert(!std::is_array<Type>::value, "be_val invalid type: array");
   static_assert(!std::is_class<Type>::value, "be_val invalid type: class/struct");
   static_assert(!std::is_pointer<Type>::value, "be_val invalid type: pointer");
   static_assert(!std::is_reference<Type>::value, "be_val invalid type: reference");
   static_assert(!std::is_union<Type>::value, "be_val invalid type: union");

   Type value() const
   {
      return byte_swap(mValue);
   }

   operator Type() const
   {
      return value();
   }

   be_val &operator=(const Type &rhs)
   {
      mValue = byte_swap(rhs);
      return *this;
   }

   // TODO: Full list of operators!
   be_val &operator++()
   {
      ++mValue;
      return *this;
   }

   be_val &operator--()
   {
      --mValue;
      return *this;
   }

protected:
   Type mValue;
};

template<typename Type, bool BigEndian = false>
class p32
{
public:
   p32()
   {
   }
   
   template<typename OtherType, bool OtherEndian>
   p32(const p32<OtherType, OtherEndian> &rhs)
   {
      *this = rhs;
   }

   Type *getPointer() const
   {
      auto address = BigEndian ? byte_swap(mValue) : mValue;
      return reinterpret_cast<Type*>(gMemory.translate(address));
   }

   void setPointer(addr_t address)
   {
      mValue = address;
   }

   Type *operator->() const
   {
      return getPointer();
   }

   p32 &operator=(const std::nullptr_t&)
   {
      mValue = 0;
      return *this;
   }

   p32 &operator=(Type *ptr)
   {
      mValue = ptr ? gMemory.untranslate(ptr) : 0;
      return *this;
   }

   p32 &operator=(const Type *ptr)
   {
      mValue = ptr ? gMemory.untranslate(ptr) : 0;
      return *this;
   }

   template<typename OtherType, bool OtherEndian>
   p32 &operator=(const p32<OtherType, OtherEndian> &rhs)
   {
      mValue = static_cast<addr_t>(rhs);
      return *this;
   }

   operator Type *() const
   {
      return getPointer();
   }

   explicit operator bool() const
   {
      return !!mValue;
   }

   explicit operator addr_t() const
   {
      return mValue;
   }

protected:
   addr_t mValue;
};

template<typename Type>
using be_ptr = p32<Type, true>;

template<typename Type>
inline p32<Type>
make_p32(const Type *src)
{
   p32<Type> result;
   result = src;
   return result;
}

template<typename Type>
inline p32<Type>
make_p32(uint32_t vaddr)
{
   p32<Type> result;
   result.setPointer(vaddr);
   return result;
}

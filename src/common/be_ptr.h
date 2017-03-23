#pragma once
#include "byte_swap.h"

#include <cstdint>
#include <libcpu/mem.h>

template<typename Type>
class be_ptr
{
public:
   be_ptr() :
      mAddress(0)
   {
   }

   be_ptr(Type *value)
   {
      setPointer(value);
   }

   Type *
   get() const
   {
      return reinterpret_cast<Type *>(mem::translate(getAddress()));
   }

   ppcaddr_t
   getAddress() const
   {
      return byte_swap(mAddress);
   }

   void
   setAddress(ppcaddr_t address)
   {
      mAddress = byte_swap(address);
   }

   void
   setPointer(Type *pointer)
   {
      setAddress(mem::untranslate(pointer));
   }

   Type *operator ->() const
   {
      return get();
   }

   operator Type *() const
   {
      return get();
   }

   template<typename OtherType>
   explicit operator be_ptr<OtherType>() const
   {
      return be_ptr<OtherType>(reinterpret_cast<OtherType *>(get()));
   }

   explicit operator bool() const
   {
      return !!mAddress;
   }

   be_ptr &
   operator =(Type *pointer)
   {
      setPointer(pointer);
      return *this;
   }

   be_ptr &
   operator =(be_ptr rhs)
   {
      setAddress(rhs.getAddress());
      return *this;
   }

   template<typename T>
   be_ptr
   operator +(T offset) const
   {
      return { mem::translate<Type>(getAddress() + static_cast<uint32_t>(offset * sizeof(Type))) };
   }

   template<typename T>
   be_ptr
   operator -(T offset) const
   {
      return { mem::translate<Type>(getAddress() - static_cast<uint32_t>(offset * sizeof(Type))) };
   }

   template<typename T>
   be_ptr &
   operator +=(T offset)
   {
      *this = *this + offset;
      return *this;
   }

   template<typename T>
   be_ptr &
   operator -=(T offset)
   {
      *this = *this - offset;
      return *this;
   }

private:
   ppcaddr_t mAddress;
};

#pragma once
#include <ostream>
#include "be_val.h"
#include "memory.h"

template<typename Type, bool BigEndian = false>
class p32
{
public:
   p32()
   {
   }

   p32(std::nullptr_t) :
      mValue(0)
   {
   }

   p32(Type *value)
   {
      setPointer(value);
   }

   template<typename OtherType, bool OtherEndian>
   p32(const p32<OtherType, OtherEndian> &rhs)
   {
      setPointer(rhs.getPointer());
   }

   Type *getPointer() const
   {
      auto address = BigEndian ? byte_swap(mValue) : mValue;
      return reinterpret_cast<Type*>(gMemory.translate(address));
   }

   void setPointer(uint32_t address)
   {
      mValue = BigEndian ? byte_swap(address) : address;
   }

   void setPointer(const void *ptr)
   {
      auto address = gMemory.untranslate(ptr);
      mValue = BigEndian ? byte_swap(address) : address;
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
      setPointer(ptr);
      return *this;
   }

   template<typename OtherType, bool OtherEndian>
   p32 &operator=(const p32<OtherType, OtherEndian> &rhs)
   {
      setPointer(rhs.getPointer());
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

   explicit operator uint32_t() const
   {
      return BigEndian ? byte_swap(mValue) : mValue;
   }

   bool operator==(uint32_t addr) const
   {
      return mValue == addr;
   }

   bool operator==(std::nullptr_t) const
   {
      return mValue == 0;
   }

protected:
   uint32_t mValue;
};

template<typename Type>
using be_ptr = p32<Type, true>;

template<typename Type>
inline p32<Type>
make_p32(Type *src)
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

template<typename Type, bool BigEndian>
static inline std::ostream&
operator<<(std::ostream& os, const p32<Type, BigEndian>& val)
{
   return os << static_cast<uint32_t>(val);
}

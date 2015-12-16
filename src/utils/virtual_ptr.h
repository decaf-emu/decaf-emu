#pragma once
#include "types.h"
#include "memory_translate.h"
#include "utils/byte_swap.h"

template<typename Type, bool IsBigEndian = false>
class virtual_ptr;

template<typename Type, bool IsBigEndian = false>
virtual_ptr<Type, IsBigEndian>
make_virtual_ptr(Type *pointer);

template<typename Type, bool IsBigEndian = false>
virtual_ptr<Type, IsBigEndian>
make_virtual_ptr(ppcaddr_t address);

// A pointer to virtual memory space (the Wii U system memory)
template<typename Type, bool IsBigEndian>
class virtual_ptr
{
public:
   virtual_ptr() :
      mAddress(0)
   {
   }

   virtual_ptr(Type *value)
   {
      setPointer(value);
   }

   template<bool OtherEndian>
   virtual_ptr(const virtual_ptr<Type, OtherEndian> &value)
   {
      setAddress(value.getAddress());
   }

   Type *get() const
   {
      return reinterpret_cast<Type *>(memory_translate(getAddress()));
   }

   ppcaddr_t getAddress() const
   {
      if (IsBigEndian) {
         return byte_swap(mAddress);
      } else {
         return mAddress;
      }
   }

   void setAddress(ppcaddr_t address)
   {
      if (IsBigEndian) {
         mAddress = byte_swap(address);
      } else {
         mAddress = address;
      }
   }

   void setPointer(Type *pointer)
   {
      setAddress(memory_untranslate(pointer));
   }

   Type *operator ->() const
   {
      return get();
   }

   operator Type *() const
   {
      return get();
   }

   explicit operator bool() const
   {
      return !!mAddress;
   }

   virtual_ptr &operator =(Type *pointer)
   {
      setPointer(pointer);
      return *this;
   }

   virtual_ptr &operator =(virtual_ptr rhs)
   {
      setAddress(rhs.getAddress());
      return *this;
   }

   template<bool OtherEndian>
   virtual_ptr &operator =(const virtual_ptr<Type, OtherEndian> &rhs)
   {
      setAddress(rhs.getAddress());
      return *this;
   }

   template<typename T>
   virtual_ptr operator +(T offset) const
   {
      return { static_cast<ppcaddr_t>(getAddress() + offset * sizeof(Type)) };
   }

   template<typename T>
   virtual_ptr operator -(T offset) const
   {
      return { static_cast<ppcaddr_t>(getAddress() - offset * sizeof(Type)) };
   }

   template<typename T>
   virtual_ptr &operator +=(T offset)
   {
      *this = *this + offset;
      return *this;
   }

   template<typename T>
   virtual_ptr &operator -=(T offset)
   {
      *this = *this - offset;
      return *this;
   }

   template<bool OtherEndian>
   bool operator ==(const virtual_ptr<Type, OtherEndian> &rhs) const
   {
      return getAddress() == rhs.getAddress();
   }

   template<bool OtherEndian>
   bool operator !=(const virtual_ptr<Type, OtherEndian> &rhs) const
   {
      return getAddress() != rhs.getAddress();
   }

   template<bool OtherEndian>
   bool operator <=(const virtual_ptr<Type, OtherEndian> &rhs) const
   {
      return getAddress() <= rhs.getAddress();
   }

   template<bool OtherEndian>
   bool operator <(const virtual_ptr<Type, OtherEndian> &rhs) const
   {
      return getAddress() < rhs.getAddress();
   }

   template<bool OtherEndian>
   bool operator >=(const virtual_ptr<Type, OtherEndian> &rhs) const
   {
      return getAddress() >= rhs.getAddress();
   }

   template<bool OtherEndian>
   bool operator >(const virtual_ptr<Type, OtherEndian> &rhs) const
   {
      return getAddress() > rhs.getAddress();
   }

private:
   friend virtual_ptr make_virtual_ptr<Type, IsBigEndian>(ppcaddr_t address);

   virtual_ptr(ppcaddr_t address)
   {
      setAddress(address);
   }

private:
   ppcaddr_t mAddress;
};

template<typename Type>
using be_ptr = virtual_ptr<Type, true>;

template<typename Type, bool IsBigEndian>
virtual_ptr<Type, IsBigEndian> make_virtual_ptr(Type *pointer)
{
   return { pointer };
}

template<typename Type, bool IsBigEndian>
virtual_ptr<Type, IsBigEndian> make_virtual_ptr(ppcaddr_t address)
{
   return { address };
}

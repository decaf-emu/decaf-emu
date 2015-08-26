#pragma once
#include "modules/coreinit/coreinit_memheap.h"

template<typename Type>
class be_data
{
public:
   be_data() {
      // TODO: Use a stack allocator instead
      mPtr = reinterpret_cast<be_val<Type>*>(OSAllocFromSystem(sizeof(Type)));
   }

   ~be_data() {
      OSFreeToSystem(mPtr);
   }

   static_assert(!std::is_array<Type>::value, "be_data invalid type: array");
   static_assert(!std::is_class<Type>::value, "be_data invalid type: class/struct");
   static_assert(!std::is_pointer<Type>::value, "be_data invalid type: pointer");
   static_assert(!std::is_reference<Type>::value, "be_data invalid type: reference");
   static_assert(!std::is_union<Type>::value, "be_data invalid type: union");

   Type value() const
   {
      return mPtr->value();
   }

   be_val<Type>* operator&()
   {
      return mPtr;
   }

   operator Type() const
   {
      return value();
   }

   template<typename Other> std::enable_if_t<std::is_assignable<Type&, Other>::value, be_data&>
   operator =(const Other &rhs)
   {
      *mPtr = rhs;
      return *this;
   }

   be_data &operator++() { *this = value() + 1; return *this; }
   be_data &operator--() { *this = value() - 1; return *this; }
   be_data operator--(int) { auto old = *this; *this = value() - 1; return old; }
   be_data operator++(int) { auto old = *this; *this = value() + 1; return old; }

   template<typename Other> bool operator == (Other rhs) const { return value() == rhs; }
   template<typename Other> bool operator != (Other rhs) const { return value() != rhs; }
   template<typename Other> bool operator >= (Other rhs) const { return value() >= rhs; }
   template<typename Other> bool operator <= (Other rhs) const { return value() <= rhs; }
   template<typename Other> bool operator > (Other rhs) const { return value() > rhs; }
   template<typename Other> bool operator < (Other rhs) const { return value() < rhs; }

   template<typename Other> be_data &operator+=(Other rhs) { *this = static_cast<Type>(value() + rhs); return *this; }
   template<typename Other> be_data &operator-=(Other rhs) { *this = static_cast<Type>(value() - rhs); return *this; }
   template<typename Other> be_data &operator*=(Other rhs) { *this = static_cast<Type>(value() * rhs); return *this; }
   template<typename Other> be_data &operator/=(Other rhs) { *this = static_cast<Type>(value() / rhs); return *this; }
   template<typename Other> be_data &operator%=(Other rhs) { *this = static_cast<Type>(value() % rhs); return *this; }
   template<typename Other> be_data &operator|=(Other rhs) { *this = static_cast<Type>(value() | rhs); return *this; }
   template<typename Other> be_data &operator&=(Other rhs) { *this = static_cast<Type>(value() & rhs); return *this; }
   template<typename Other> be_data &operator^=(Other rhs) { *this = static_cast<Type>(value() ^ rhs); return *this; }

protected:
   be_val<Type> *mPtr;

};
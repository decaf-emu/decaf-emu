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

   template<typename Other> std::enable_if_t<std::is_assignable<Type&, Other>::value, be_val&>
   operator =(const Other& rhs)
   {
      mValue = byte_swap(rhs);
      return *this;
   }

   // TODO: Full list of operators!
   be_val &operator++()
   {
      *this = value() + 1;
      return *this;
   }

   be_val &operator--()
   {
      *this = value() - 1;
      return *this;
   }

   be_val operator--(int)
   {
      auto old = *this;
      *this = value() - 1;
      return old;
   }

   be_val operator++(int)
   {
      auto old = *this;
      *this = value() + 1;
      return old;
   }

   template<typename Other>
   be_val &operator|=(Other rhs)
   {
      *this = static_cast<Type>(value() | rhs);
      return *this;
   }

   template<typename Other>
   be_val &operator&=(Other rhs)
   {
      *this = static_cast<Type>(value() & rhs);
      return *this;
   }

protected:
   Type mValue;
};

#pragma once
#include <ostream>
#include <type_traits>
#include "utils/byte_swap.h"

template<typename Type>
class be_val
{
public:
   static_assert(!std::is_array<Type>::value, "be_val invalid type: array");
   static_assert(!std::is_class<Type>::value, "be_val invalid type: class/struct");
   static_assert(!std::is_pointer<Type>::value, "be_val invalid type: pointer");
   static_assert(!std::is_reference<Type>::value, "be_val invalid type: reference");
   static_assert(!std::is_union<Type>::value, "be_val invalid type: union");

   be_val()
   {
   }

   be_val(Type value)
   {
      *this = value;
   }

   Type value() const
   {
      return byte_swap(mValue);
   }

   operator Type() const
   {
      return value();
   }

   template<typename Other> std::enable_if_t<std::is_assignable<Type&, Other>::value, be_val&>
   operator =(const Other &rhs)
   {
      mValue = byte_swap(static_cast<Type>(rhs));
      return *this;
   }

   be_val &operator++() { *this = value() + 1; return *this; }
   be_val &operator--() { *this = value() - 1; return *this; }
   be_val operator--(int) { auto old = *this; *this = value() - 1; return old; }
   be_val operator++(int) { auto old = *this; *this = value() + 1; return old; }

   template<typename Other> bool operator == (const Other &rhs) const { return value() == static_cast<Type>(rhs); }
   template<typename Other> bool operator != (const Other &rhs) const { return value() != static_cast<Type>(rhs); }
   template<typename Other> bool operator >= (const Other &rhs) const { return value() >= static_cast<Type>(rhs); }
   template<typename Other> bool operator <= (const Other &rhs) const { return value() <= static_cast<Type>(rhs); }
   template<typename Other> bool operator  > (const Other &rhs) const { return value()  > static_cast<Type>(rhs); }
   template<typename Other> bool operator  < (const Other &rhs) const { return value()  < static_cast<Type>(rhs); }

   template<typename Other> be_val &operator+=(const Other &rhs) { *this = static_cast<Type>(value() + rhs); return *this; }
   template<typename Other> be_val &operator-=(const Other &rhs) { *this = static_cast<Type>(value() - rhs); return *this; }
   template<typename Other> be_val &operator*=(const Other &rhs) { *this = static_cast<Type>(value() * rhs); return *this; }
   template<typename Other> be_val &operator/=(const Other &rhs) { *this = static_cast<Type>(value() / rhs); return *this; }
   template<typename Other> be_val &operator%=(const Other &rhs) { *this = static_cast<Type>(value() % rhs); return *this; }
   template<typename Other> be_val &operator|=(const Other &rhs) { *this = static_cast<Type>(value() | rhs); return *this; }
   template<typename Other> be_val &operator&=(const Other &rhs) { *this = static_cast<Type>(value() & rhs); return *this; }
   template<typename Other> be_val &operator^=(const Other &rhs) { *this = static_cast<Type>(value() ^ rhs); return *this; }

   template<typename Other> Type operator+(const Other &rhs) const { return static_cast<Type>(value() + rhs); }
   template<typename Other> Type operator-(const Other &rhs) const { return static_cast<Type>(value() - rhs); }
   template<typename Other> Type operator*(const Other &rhs) const { return static_cast<Type>(value() * rhs); }
   template<typename Other> Type operator/(const Other &rhs) const { return static_cast<Type>(value() / rhs); }
   template<typename Other> Type operator%(const Other &rhs) const { return static_cast<Type>(value() % rhs); }
   template<typename Other> Type operator|(const Other &rhs) const { return static_cast<Type>(value() | rhs); }
   template<typename Other> Type operator&(const Other &rhs) const { return static_cast<Type>(value() & rhs); }
   template<typename Other> Type operator^(const Other &rhs) const { return static_cast<Type>(value() ^ rhs); }

protected:
   Type mValue {};
};

template<typename Type>
static inline std::ostream&
operator<<(std::ostream& os, const be_val<Type>& val)
{
   return os << static_cast<Type>(val);
}

#pragma once
#include <ostream>
#include <type_traits>
#include "utils/byte_swap.h"

// Should be identical to be_val except volatile mValue
template<typename Type>
class be_volatile
{
public:
   static_assert(!std::is_array<Type>::value, "be_volatile invalid type: array");
   static_assert(!std::is_class<Type>::value, "be_volatile invalid type: class/struct");
   static_assert(!std::is_pointer<Type>::value, "be_volatile invalid type: pointer");
   static_assert(!std::is_reference<Type>::value, "be_volatile invalid type: reference");
   static_assert(!std::is_union<Type>::value, "be_volatile invalid type: union");

   be_volatile()
   {
   }

   be_volatile(Type value)
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

   template<typename Other> std::enable_if_t<std::is_assignable<Type&, Other>::value, be_volatile&>
   operator =(const Other &rhs)
   {
      mValue = byte_swap(static_cast<Type>(rhs));
      return *this;
   }

   be_volatile &operator++() { *this = value() + 1; return *this; }
   be_volatile &operator--() { *this = value() - 1; return *this; }
   be_volatile operator--(int) { auto old = *this; *this = value() - 1; return old; }
   be_volatile operator++(int) { auto old = *this; *this = value() + 1; return old; }

   template<typename Other> bool operator == (const Other &rhs) const { return value() == static_cast<Type>(rhs); }
   template<typename Other> bool operator != (const Other &rhs) const { return value() != static_cast<Type>(rhs); }
   template<typename Other> bool operator >= (const Other &rhs) const { return value() >= static_cast<Type>(rhs); }
   template<typename Other> bool operator <= (const Other &rhs) const { return value() <= static_cast<Type>(rhs); }
   template<typename Other> bool operator  > (const Other &rhs) const { return value()  > static_cast<Type>(rhs); }
   template<typename Other> bool operator  < (const Other &rhs) const { return value()  < static_cast<Type>(rhs); }

   template<typename Other> be_volatile &operator+=(const Other &rhs) { *this = static_cast<Type>(value() + rhs); return *this; }
   template<typename Other> be_volatile &operator-=(const Other &rhs) { *this = static_cast<Type>(value() - rhs); return *this; }
   template<typename Other> be_volatile &operator*=(const Other &rhs) { *this = static_cast<Type>(value() * rhs); return *this; }
   template<typename Other> be_volatile &operator/=(const Other &rhs) { *this = static_cast<Type>(value() / rhs); return *this; }
   template<typename Other> be_volatile &operator%=(const Other &rhs) { *this = static_cast<Type>(value() % rhs); return *this; }
   template<typename Other> be_volatile &operator|=(const Other &rhs) { *this = static_cast<Type>(value() | rhs); return *this; }
   template<typename Other> be_volatile &operator&=(const Other &rhs) { *this = static_cast<Type>(value() & rhs); return *this; }
   template<typename Other> be_volatile &operator^=(const Other &rhs) { *this = static_cast<Type>(value() ^ rhs); return *this; }

   template<typename Other> Type operator+(const Other &rhs) const { return static_cast<Type>(value() + rhs); }
   template<typename Other> Type operator-(const Other &rhs) const { return static_cast<Type>(value() - rhs); }
   template<typename Other> Type operator*(const Other &rhs) const { return static_cast<Type>(value() * rhs); }
   template<typename Other> Type operator/(const Other &rhs) const { return static_cast<Type>(value() / rhs); }
   template<typename Other> Type operator%(const Other &rhs) const { return static_cast<Type>(value() % rhs); }
   template<typename Other> Type operator|(const Other &rhs) const { return static_cast<Type>(value() | rhs); }
   template<typename Other> Type operator&(const Other &rhs) const { return static_cast<Type>(value() & rhs); }
   template<typename Other> Type operator^(const Other &rhs) const { return static_cast<Type>(value() ^ rhs); }

protected:
   volatile Type mValue {};
};

template<typename Type>
static inline std::ostream&
operator<<(std::ostream& os, const be_volatile<Type>& val)
{
   return os << static_cast<Type>(val);
}

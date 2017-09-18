#pragma once
#include "address.h"

#include <common/byte_swap.h>
#include <ostream>

namespace cpu
{

template<typename ValueType, typename AddressType>
class Pointer;

template<typename Value>
using VirtualPointer = Pointer<Value, VirtualAddress>;

template<typename Type>
class BigEndianValue
{
public:
   static_assert(!std::is_array<Type>::value, "be_val invalid type: array");
   static_assert(!std::is_pointer<Type>::value, "be_val invalid type: pointer");
   static_assert(sizeof(Type) == 1 || sizeof(Type) == 2 || sizeof(Type) == 4 || sizeof(Type) == 8, "be_val invalid type size");
   using value_type = Type;

   BigEndianValue() = default;

   BigEndianValue(const value_type &value) :
      mStorage(byte_swap(value))
   {
   }

   value_type value() const
   {
      return byte_swap(mStorage);
   }

   void setValue(value_type value)
   {
      mStorage = byte_swap(value);
   }

   operator value_type() const
   {
      return value();
   }

   template<typename = typename std::enable_if<std::is_convertible<value_type, bool>::value>::type>
   explicit operator bool() const
   {
      return static_cast<bool>(value());
   }

   template<typename OtherType, typename = typename std::enable_if<std::is_constructible<value_type, const OtherType &>::value ||
                                                                   std::is_convertible<const OtherType &, value_type>::value>::type>
   BigEndianValue & operator =(const OtherType &other)
   {
      if constexpr (std::is_constructible<value_type, const OtherType &>::value) {
         setValue(value_type { other });
      } else {
         setValue(static_cast<value_type>(other));
      }

      return *this;
   }

   template<typename OtherType, typename = typename std::enable_if<std::is_convertible<const OtherType &, value_type>::value ||
                                                                   std::is_constructible<value_type, const OtherType &>::value>::type>
   BigEndianValue & operator =(const BigEndianValue<OtherType> &other)
   {
      if constexpr (std::is_constructible<value_type, const OtherType &>::value) {
         setValue(value_type { other.value() });
      } else {
         setValue(static_cast<value_type>(other.value()));
      }

      return *this;
   }

   template<typename OtherType, typename = typename std::enable_if<std::is_convertible<Type, OtherType>::value ||
                                                                   std::is_constructible<OtherType, Type>::value>::type>
   explicit operator OtherType() const
   {
      if constexpr (std::is_constructible<OtherType, value_type>::value) {
         return OtherType { value() };
      } else {
         return static_cast<OtherType>(value());
      }
   }

   template<typename OtherType, typename K = value_type>
   auto operator ==(const OtherType &other) -> decltype(std::declval<const K>().operator ==(std::declval<const OtherType>())) const
   {
      return value() == other;
   }

   template<typename OtherType, typename K = value_type>
   auto operator !=(const OtherType &other) -> decltype(std::declval<const K>().operator !=(std::declval<const OtherType>())) const
   {
      return value() != other;
   }

   template<typename OtherType, typename K = value_type>
   auto operator >=(const OtherType &other) -> decltype(std::declval<const K>().operator >=(std::declval<const OtherType>())) const
   {
      return value() >= other;
   }

   template<typename OtherType, typename K = value_type>
   auto operator <=(const OtherType &other) -> decltype(std::declval<const K>().operator <=(std::declval<const OtherType>())) const
   {
      return value() <= other;
   }

   template<typename OtherType, typename K = value_type>
   auto operator >(const OtherType &other) -> decltype(std::declval<const K>().operator >(std::declval<const OtherType>())) const
   {
      return value() > other;
   }

   template<typename OtherType, typename K = value_type>
   auto operator <(const OtherType &other) -> decltype(std::declval<const K>().operator <(std::declval<const OtherType>())) const
   {
      return value() < other;
   }

   template<typename K = value_type>
   auto operator +() ->  decltype(std::declval<const K>(). operator+()) const
   {
      return +value();
   }

   template<typename K = value_type>
   auto operator -() ->  decltype(std::declval<const K>(). operator-()) const
   {
      return -value();
   }

   template<typename OtherType, typename K = value_type>
   auto operator +(const OtherType &other) -> decltype(std::declval<const K>().operator +(std::declval<const OtherType>())) const
   {
      return value() + other;
   }

   template<typename OtherType, typename K = value_type>
   auto operator -(const OtherType &other) -> decltype(std::declval<const K>().operator -(std::declval<const OtherType>())) const
   {
      return value() - other;
   }

   template<typename OtherType, typename K = value_type>
   auto operator *(const OtherType &other) -> decltype(std::declval<const K>().operator *(std::declval<const OtherType>())) const
   {
      return value() * other;
   }

   template<typename OtherType, typename K = value_type>
   auto operator /(const OtherType &other) -> decltype(std::declval<const K>().operator /(std::declval<const OtherType>())) const
   {
      return value() / other;
   }

   template<typename OtherType, typename K = value_type>
   auto operator %(const OtherType &other) -> decltype(std::declval<const K>().operator %(std::declval<const OtherType>())) const
   {
      return value() % other;
   }

   template<typename OtherType, typename K = value_type>
   auto operator |(const OtherType &other) -> decltype(std::declval<const K>().operator |(std::declval<const OtherType>())) const
   {
      return value() | other;
   }

   template<typename OtherType, typename K = value_type>
   auto operator &(const OtherType &other) -> decltype(std::declval<const K>().operator &(std::declval<const OtherType>())) const
   {
      return value() & other;
   }

   template<typename OtherType, typename K = value_type>
   auto operator ^(const OtherType &other) -> decltype(std::declval<const K>().operator ^(std::declval<const OtherType>())) const
   {
      return value() ^ other;
   }

   template<typename OtherType, typename K = value_type>
   auto operator <<(const OtherType &other) -> decltype(std::declval<const K>().operator <<(std::declval<const OtherType>())) const
   {
      return value() << other;
   }

   template<typename OtherType, typename K = value_type>
   auto operator >>(const OtherType &other) -> decltype(std::declval<const K>().operator >>(std::declval<const OtherType>())) const
   {
      return value() >> other;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() + std::declval<const OtherType>())>
   BigEndianValue &operator +=(const OtherType &other)
   {
      *this = value() + other;
      return *this;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() - std::declval<const OtherType>())>
   BigEndianValue &operator -=(const OtherType &other)
   {
      *this = value() - other;
      return *this;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() * std::declval<const OtherType>())>
   BigEndianValue &operator *=(const OtherType &other)
   {
      *this = value() * other;
      return *this;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() / std::declval<const OtherType>())>
   BigEndianValue &operator /=(const OtherType &other)
   {
      *this = value() / other;
      return *this;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() % std::declval<const OtherType>())>
   BigEndianValue &operator %=(const OtherType &other)
   {
      *this = value() % other;
      return *this;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() | std::declval<const OtherType>())>
   BigEndianValue &operator |=(const OtherType &other)
   {
      *this = static_cast<Type>(value() | other);
      return *this;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() & std::declval<const OtherType>())>
   BigEndianValue &operator &=(const OtherType &other)
   {
      *this = static_cast<Type>(value() & other);
      return *this;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() ^ std::declval<const OtherType>())>
   BigEndianValue &operator ^=(const OtherType &other)
   {
      *this = static_cast<Type>(value() ^ other);
      return *this;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() << std::declval<const OtherType>())>
   BigEndianValue &operator <<=(const OtherType &other)
   {
      *this = value() << other;
      return *this;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() >> std::declval<const OtherType>())>
   BigEndianValue &operator >>=(const OtherType &other)
   {
      *this = value() >> other;
      return *this;
   }

   template<typename T = Type, typename = decltype(std::declval<const T>() + 1)>
   BigEndianValue &operator ++()
   {
      setValue(value() + 1);
      return *this;
   }

   template<typename T = Type, typename = decltype(std::declval<const T>() + 1)>
   BigEndianValue operator ++(int)
   {
      auto before = *this;
      setValue(value() + 1);
      return before;
   }

   template<typename T = Type, typename = decltype(std::declval<const T>() - 1)>
   BigEndianValue &operator --()
   {
      setValue(value() - 1);
      return *this;
   }

   template<typename T = Type, typename = decltype(std::declval<const T>() - 1)>
   BigEndianValue operator --(int)
   {
      auto before = *this;
      setValue(value() - 1);
      return before;
   }

   template<typename IndexType>
   auto operator [](const IndexType &index)
   {
      return value().operator [](index);
   }

   template<typename K = value_type>
   auto operator ->() -> decltype(std::declval<K>().operator ->())
   {
      return value().operator ->();
   }

   template<typename K = value_type>
   auto operator ->() const -> decltype(std::declval<const K>().operator ->())
   {
      return value().operator ->();
   }

   template<typename K = value_type>
   auto operator *() -> decltype(std::declval<K>().operator *())
   {
      return value().operator *();
   }

   template<typename K = value_type>
   auto operator *() const -> decltype(std::declval<const K>().operator *())
   {
      return value().operator ->();
   }

   template<typename K = value_type>
   auto getAddress() -> decltype(std::declval<const K>().getAddress()) const
   {
      return value().getAddress();
   }

   template<typename K = value_type>
   auto getRawPointer() -> decltype(std::declval<const K>().getRawPointer()) const
   {
      return value().getRawPointer();
   }

   // Please use virt_addrof or phys_addrof instead
   VirtualPointer<Type> operator &() = delete;

private:
   value_type mStorage;
};

template<typename Type>
inline std::ostream &
operator <<(std::ostream &os, const BigEndianValue<Type> &val)
{
   return os << val.value();
}

} // namespace cpu

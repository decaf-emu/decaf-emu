#pragma once
#include <common/byte_swap.h>

namespace cpu
{

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

   template<typename = typename std::enable_if<std::is_constructible<bool, value_type>::value>::type>
   explicit operator bool() const
   {
      return static_cast<bool>(value());
   }

   template<typename OtherType, typename = typename std::enable_if<std::is_constructible<value_type, const OtherType &>::value>::type>
   BigEndianValue & operator =(const OtherType &other)
   {
      setValue(value_type { other });
      return *this;
   }

   template<typename OtherType, typename = typename std::enable_if<std::is_constructible<value_type, const OtherType &>::value>::type>
   BigEndianValue & operator =(const BigEndianValue<OtherType> &other)
   {
      setValue(value_type { other.value() });
      return *this;
   }

   template<typename OtherType, typename = typename std::enable_if<std::is_constructible<OtherType, Type>::value>::type>
   explicit operator OtherType() const
   {
      return static_cast<OtherType>(value());
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() == std::declval<const OtherType>())>
   bool operator ==(const OtherType &other) const
   {
      return value() == other;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() != std::declval<const OtherType>())>
   bool operator !=(const OtherType &other) const
   {
      return value() != other;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() >= std::declval<const OtherType>())>
   bool operator >=(const OtherType &other) const
   {
      return value() >= other;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() <= std::declval<const OtherType>())>
   bool operator <=(const OtherType &other) const
   {
      return value() <= other;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() > std::declval<const OtherType>())>
   bool operator >(const OtherType &other) const
   {
      return value() > other;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() < std::declval<const OtherType>())>
   bool operator <(const OtherType &other) const
   {
      return value() < other;
   }

   template<typename = decltype(+std::declval<const value_type>())>
   value_type operator +() const
   {
      return +value();
   }

   template<typename = decltype(-std::declval<const value_type>())>
   value_type operator -() const
   {
      return -value();
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() + std::declval<const OtherType>())>
   value_type operator +(const OtherType &other) const
   {
      return value() + other;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() - std::declval<const OtherType>())>
   value_type operator -(const OtherType &other) const
   {
      return value() - other;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() * std::declval<const OtherType>())>
   value_type operator *(const OtherType &other) const
   {
      return value() * other;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() / std::declval<const OtherType>())>
   value_type operator /(const OtherType &other) const
   {
      return value() / other;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() % std::declval<const OtherType>())>
   value_type operator %(const OtherType &other) const
   {
      return value() % other;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() | std::declval<const OtherType>())>
   value_type operator |(const OtherType &other) const
   {
      return static_cast<Type>(value() | other);
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() & std::declval<const OtherType>())>
   value_type operator &(const OtherType &other) const
   {
      return static_cast<Type>(value() & other);
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() ^ std::declval<const OtherType>())>
   value_type operator ^(const OtherType &other) const
   {
      return static_cast<Type>(value() ^ other);
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() << std::declval<const OtherType>())>
   value_type operator <<(const OtherType &other) const
   {
      return value() << other;
   }

   template<typename OtherType, typename = decltype(std::declval<const value_type>() >> std::declval<const OtherType>())>
   value_type operator >>(const OtherType &other) const
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

   template<typename = decltype(std::declval<const value_type>() + 1)>
   BigEndianValue &operator ++()
   {
      *this = value() + 1;
      return *this;
   }

   template<typename = decltype(std::declval<const value_type>() + 1)>
   BigEndianValue operator ++(int)
   {
      auto before = *this;
      *this = value() + 1;
      return before;
   }

   template<typename = decltype(std::declval<const value_type>() - 1)>
   BigEndianValue &operator --()
   {
      *this = value() - 1;
      return *this;
   }

   template<typename = decltype(std::declval<const value_type>() - 1)>
   BigEndianValue operator --(int)
   {
      auto before = *this;
      *this = value() - 1;
      return before;
   }

   template<typename IndexType>
   auto operator [](const IndexType &index)
   {
      return value().operator [](index);
   }

   template<typename = decltype(std::declval<value_type>().operator ->())>
   auto operator ->()
   {
      return value().operator ->();
   }

   template<typename = decltype(std::declval<const value_type>().operator ->())>
   auto operator ->() const
   {
      return value().operator ->();
   }

   template<typename = decltype(std::declval<value_type>().operator *())>
   auto operator *()
   {
      return value().operator *();
   }

   template<typename = decltype(std::declval<const value_type>().operator *())>
   auto operator *() const
   {
      return value().operator *();
   }

private:
   value_type mStorage;
};

} // namespace cpu

#pragma once
#include "address.h"
#include "be2_val.h"
#include "mmu.h"

#include <fmt/format.h>
#include <type_traits>

template<typename Type>
struct be2_struct;

namespace cpu
{

template<typename ValueType, typename AddressType>
class Pointer;

template<typename AddressType, typename SrcType, typename DstType>
struct pointer_cast_impl;

template<typename>
struct is_big_endian_value : std::false_type { };

template<typename ValueType>
struct is_big_endian_value<be2_val<ValueType>> : std::true_type { };

template<typename>
struct is_cpu_pointer : std::false_type { };

template<typename ValueType, typename AddressType>
struct is_cpu_pointer<Pointer<ValueType, AddressType>> : std::true_type { };

template<typename>
struct is_cpu_address : std::false_type { };

template<typename AddressType>
struct is_cpu_address<Address<AddressType>> : std::true_type { };

template <typename T, typename = void>
struct pointer_dereference_type;

template <typename T>
struct pointer_dereference_type<T, typename std::enable_if<std::is_void<T>::value>::type>
{
   using type = std::nullptr_t;
};

/*
 * If Type is an arithmetic type, enum type, or cpu::Pointer<> type, then we
 * must dereference to a be2_val.
 */
template <typename T>
struct pointer_dereference_type<T, typename std::enable_if<(std::is_arithmetic<T>::value
                                                        || std::is_enum<T>::value
                                                        || is_cpu_pointer<T>::value
                                                        || is_cpu_address<T>::value)
                                                        && !std::is_const<T>::value>::type>
{
   using type = be2_val<T>;
};

/*
 * const version of above
 */
template <typename T>
struct pointer_dereference_type<T, typename std::enable_if<(std::is_arithmetic<T>::value
                                                        || std::is_enum<T>::value
                                                        || is_cpu_pointer<T>::value
                                                        || is_cpu_address<T>::value)
                                                        && std::is_const<T>::value>::type>
{
   using type = const be2_val<T>;
};

/*
 * If Type is an class type, or union type, and NOT a cpu::Pointer<> type, then
 * we must dereference to be2_struct<Type>.
 */
template <typename T>
struct pointer_dereference_type<T, typename std::enable_if<(std::is_class<T>::value || std::is_union<T>::value)
                                                         && !is_cpu_pointer<T>::value
                                                         && !is_cpu_address<T>::value>::type>
{
   using type = be2_struct<T>;
};

template<typename ValueType, typename AddressType>
class Pointer
{
public:
   static_assert(!is_big_endian_value<ValueType>::value,
                 "be2_val should not be used as ValueType for pointer, it is implied implicitly.");

   static_assert(std::is_class<ValueType>::value
              || std::is_union<ValueType>::value
              || std::is_arithmetic<ValueType>::value
              || std::is_enum<ValueType>::value
              || std::is_void<ValueType>::value,
                 "Invalid ValueType for Pointer");

   using value_type = ValueType;
   using address_type = AddressType;
   using dereference_type = typename pointer_dereference_type<value_type>::type;

   Pointer() = default;
   Pointer(const Pointer &) = default;
   Pointer(Pointer &&) = default;
   Pointer &operator =(const Pointer &) = default;
   Pointer &operator =(Pointer &&) = default;

   // Pointer<Type>(nullptr)
   Pointer(std::nullptr_t) :
      mAddress(0)
   {
   }

    // Pointer<const Type>(Pointer<Type>)
   template<typename V = ValueType>
   Pointer(const Pointer<typename std::remove_const<V>::type, AddressType> &other,
           typename std::enable_if<std::is_const<V>::value>::type * = nullptr) :
      mAddress(other.mAddress)
   {
   }

   // Pointer<void>(Pointer<Type>)
   template<typename O, typename V = ValueType>
   Pointer(const Pointer<O, AddressType> &other,
           typename std::enable_if<std::is_void<V>::value>::type * = nullptr) :
      mAddress(other.mAddress)
   {
   }

   // Pointer<const Type> = const Pointer<Type> &
   template<typename V = ValueType,
            typename = std::enable_if<std::is_const<ValueType>::value>::type>
   Pointer &operator =(const Pointer<typename std::remove_const<V>::type, AddressType> &other)
   {
      mAddress = other.mAddress;
      return *this;
   }

   ValueType *getRawPointer() const
   {
      return internal::translate<ValueType>(mAddress);
   }

   explicit operator bool() const
   {
      return static_cast<bool>(mAddress);
   }

   explicit operator Pointer<void, address_type>() const
   {
      return { mAddress };
   }

   template<typename K = ValueType>
   typename std::enable_if<!std::is_void<K>::value, dereference_type &>::type operator *()
   {
      return *internal::translate<dereference_type>(mAddress);
   }

   template<typename K = ValueType>
   typename std::enable_if<!std::is_void<K>::value, const dereference_type &>::type operator *() const
   {
      return *internal::translate<const dereference_type>(mAddress);
   }

   template<typename K = ValueType>
   typename std::enable_if<!std::is_void<K>::value, dereference_type *>::type operator ->()
   {
      return internal::translate<dereference_type>(mAddress);
   }

   template<typename K = ValueType>
   typename std::enable_if<!std::is_void<K>::value, const dereference_type *>::type operator ->() const
   {
      return internal::translate<const dereference_type>(mAddress);
   }

   template<typename K = ValueType>
   typename std::enable_if<!std::is_void<K>::value, dereference_type &>::type operator [](size_t index)
   {
      return internal::translate<dereference_type>(mAddress)[index];
   }

   template<typename K = ValueType>
   typename std::enable_if<!std::is_void<K>::value, const dereference_type &>::type operator [](size_t index) const
   {
      return internal::translate<const dereference_type>(mAddress)[index];
   }

   constexpr bool operator ==(std::nullptr_t) const
   {
      return !static_cast<bool>(mAddress);
   }

   constexpr bool operator == (const Pointer &other) const
   {
      return mAddress == other.mAddress;
   }

   constexpr bool operator != (const Pointer &other) const
   {
      return mAddress != other.mAddress;
   }

   constexpr bool operator >= (const Pointer &other) const
   {
      return mAddress >= other.mAddress;
   }

   constexpr bool operator <= (const Pointer &other) const
   {
      return mAddress <= other.mAddress;
   }

   constexpr bool operator > (const Pointer &other) const
   {
      return mAddress > other.mAddress;
   }

   constexpr bool operator < (const Pointer &other) const
   {
      return mAddress < other.mAddress;
   }

   template<typename T = ValueType>
   constexpr typename std::enable_if<!std::is_void<T>::value, Pointer &>::type
   operator ++ ()
   {
      mAddress += sizeof(value_type);
      return *this;
   }

   template<typename T = ValueType>
   constexpr typename std::enable_if<!std::is_void<T>::value, Pointer &>::type
   operator += (ptrdiff_t value)
   {
      mAddress += value * sizeof(value_type);
      return *this;
   }

   template<typename T = ValueType>
   constexpr typename std::enable_if<!std::is_void<T>::value, Pointer &>::type
   operator -= (ptrdiff_t value)
   {
      mAddress -= value * sizeof(value_type);
      return *this;
   }

   template<typename T = ValueType>
   constexpr typename std::enable_if<!std::is_void<T>::value, Pointer>::type
   operator + (ptrdiff_t value) const
   {
      Pointer dst;
      dst.mAddress = mAddress + (value * sizeof(value_type));
      return dst;
   }

   template<typename T = ValueType>
   constexpr typename std::enable_if<!std::is_void<T>::value, ptrdiff_t>::type
   operator -(const Pointer &other) const
   {
      return (mAddress - other.mAddress) / sizeof(value_type);
   }

   template<typename T = ValueType>
   constexpr typename std::enable_if<!std::is_void<T>::value, Pointer>::type
   operator -(ptrdiff_t value) const
   {
      Pointer dst;
      dst.mAddress = mAddress - (value * sizeof(value_type));
      return dst;
   }

protected:
   template<typename AddressType, typename SrcType, typename DstType>
   friend struct pointer_cast_impl;

   template<typename AddressType2, typename ValueType2>
   friend class Pointer;

   address_type mAddress;
};

template<typename ValueType, typename AddressType>
static inline void
format_arg(fmt::BasicFormatter<char> &f,
           const char *&format_str,
           const Pointer<ValueType, AddressType> &val)
{
   format_str = f.format(format_str,
                         fmt::internal::MakeArg<fmt::BasicFormatter<char>>(static_cast<AddressType>(val)));
}

template<typename Value>
using VirtualPointer = Pointer<Value, VirtualAddress>;

template<typename Value>
using PhysicalPointer = Pointer<Value, PhysicalAddress>;

template<typename AddressType, typename SrcType, typename DstTypePtr>
struct pointer_cast_impl
{
   static_assert(std::is_pointer<DstTypePtr>::value);
   using DstType = typename std::remove_pointer<DstTypePtr>::type;

   // Pointer<X, AddressType> to Pointer<Y, AddressType>
   static constexpr Pointer<DstType, AddressType> cast(Pointer<SrcType, AddressType> src)
   {
      Pointer<DstType, AddressType> dst;
      dst.mAddress = src.mAddress;
      return dst;
   }
};

template<typename AddressType, typename DstTypePtr>
struct pointer_cast_impl<AddressType, AddressType, DstTypePtr>
{
   static_assert(std::is_pointer<DstTypePtr>::value);
   using DstType = typename std::remove_pointer<DstTypePtr>::type;

   // AddressType to Pointer<X, AddressType>
   static constexpr Pointer<DstType, AddressType> cast(AddressType src)
   {
      Pointer<DstType, AddressType> dst;
      dst.mAddress = src;
      return dst;
   }
};

template<typename AddressType, typename SrcType>
struct pointer_cast_impl<AddressType, SrcType, AddressType>
{
   // Pointer<X, AddressType> to AddressType
   static constexpr AddressType cast(Pointer<SrcType, AddressType> src)
   {
      return src.mAddress;
   }
};

} // namespace cpu

#pragma once
#include "address.h"
#include "bigendianvalue.h"
#include "mmu.h"

#include <fmt/format.h>
#include <type_traits>

template<typename Type>
struct be2_struct;

namespace cpu
{

template<typename Value, typename Address>
class Pointer;

template<typename>
struct is_big_endian_value : std::false_type { };

template<typename ValueType>
struct is_big_endian_value<BigEndianValue<ValueType>> : std::true_type { };

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
 * must dereference to a BigEndianValue.
 */
template <typename T>
struct pointer_dereference_type<T, typename std::enable_if<(std::is_arithmetic<T>::value
                                                        || std::is_enum<T>::value
                                                        || is_cpu_pointer<T>::value
                                                        || is_cpu_address<T>::value)
                                                        && !std::is_const<T>::value>::type>
{
   using type = BigEndianValue<T>;
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
   using type = const BigEndianValue<T>;
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
                 "BigEndianValue should not be used as ValueType for pointer, it is implied implicitly.");

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

   /**
    * Constructs a pointer from an address.
    */
   Pointer(address_type address) :
      mAddress(address)
   {
   }

   /**
    * Constructs a pointer from a BigEndianValue address.
    */
   Pointer(BigEndianValue<address_type> address) :
      mAddress(address.value())
   {
   }

   /**
    * Constructs a pointer from a nullptr
    */
   Pointer(std::nullptr_t) :
      mAddress(0)
   {
   }

   /**
    * Constructs a const pointer from a non-const pointer of same type
    * Pointer<const Type, Address> from Pointer<Type, Address>
    */
   template<typename V = ValueType, typename A = AddressType>
   Pointer(const Pointer<typename std::remove_const<V>::type, A> &other,
           typename std::enable_if<std::is_const<V>::value>::type * = nullptr) :
      mAddress(address_type { other })
   {
   }

   /**
    * Constructs a void pointer from a non-void pointer
    * Pointer<void, Address> from Pointer<OtherType, Address>
    */
   template<typename O, typename V = ValueType, typename A = AddressType>
   Pointer(const Pointer<O, A> &other,
           typename std::enable_if<std::is_void<V>::value>::type * = 0) :
      mAddress(address_type { other })
   {
   }

   ValueType *getRawPointer() const
   {
      return internal::translate<ValueType>(mAddress);
   }

   explicit operator bool() const
   {
      return static_cast<bool>(mAddress);
   }

   explicit operator address_type() const
   {
      return mAddress;
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
      return Pointer { mAddress + (value * sizeof(value_type)) };
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
      return Pointer { mAddress - (value * sizeof(value_type)) };
   }

protected:
   address_type mAddress;
};

template<typename ValueType, typename AddressType>
static inline void
format_arg(fmt::BasicFormatter<char> &f,
           const char *&format_str,
           const Pointer<ValueType, AddressType> &val)
{
   format_str = f.format(format_str,
                         fmt::internal::MakeArg<fmt::BasicFormatter<char>>(val.getRawPointer()));
}

template<typename Value>
using VirtualPointer = Pointer<Value, VirtualAddress>;

template<typename Value>
using PhysicalPointer = Pointer<Value, PhysicalAddress>;

} // namespace cpu

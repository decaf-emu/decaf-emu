#pragma once
#include "address.h"
#include "be2_val.h"
#include "mmu.h"

#include <type_traits>

template<typename Type>
struct be2_struct;

template<typename Type, uint32_t Size>
class be2_array;

namespace cpu
{

template<typename ValueType, typename AddressType>
class Pointer;

template<typename AddressType, typename FunctonType>
class FunctionPointer;

template<typename AddressType, typename SrcType, typename DstType, typename = void>
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

template<typename>
struct is_cpu_func_pointer : std::false_type { };

template<typename AddressType, typename FunctionType>
struct is_cpu_func_pointer<FunctionPointer<AddressType, FunctionType>> : std::true_type { };

template <typename T, typename = void>
struct pointer_dereference_type;

template <typename T>
struct pointer_dereference_type<T,
   typename std::enable_if<std::is_void<T>::value>::type>
{
   using type = void;
};

/*
 * 1 byte values do not need to have be2_val<> wrapped around them.
 */
template <typename T>
struct pointer_dereference_type<T,
   typename std::enable_if<(std::is_arithmetic<T>::value
                         || std::is_enum<T>::value)
                         && sizeof(T) == 1>::type>
{
   using type = T;
};

/*
 * If Type is an arithmetic type, enum type, or cpu::Pointer<> type, then we
 * must dereference to a be2_val.
 */
template <typename T>
struct pointer_dereference_type<T,
   typename std::enable_if<(std::is_arithmetic<T>::value
                         || std::is_enum<T>::value
                         || is_cpu_address<T>::value
                         || is_cpu_pointer<T>::value
                         || is_cpu_func_pointer<T>::value)
                         && !std::is_const<T>::value
                         && sizeof(T) != 1>::type>
{
   using type = be2_val<T>;
};

/*
 * const version of above
 */
template <typename T>
struct pointer_dereference_type<T,
   typename std::enable_if<(std::is_arithmetic<T>::value
                         || std::is_enum<T>::value
                         || is_cpu_address<typename std::remove_const<T>::type>::value
                         || is_cpu_pointer<typename std::remove_const<T>::type>::value
                         || is_cpu_func_pointer<typename std::remove_const<T>::type>::value)
                         && std::is_const<T>::value
                         && sizeof(T) != 1>::type>
{
   using type = const be2_val<T>;
};

/*
 * If Type is an array then we must dereference to be2_array.
 */
template <typename T>
struct pointer_dereference_type<T,
   typename std::enable_if<std::is_array<T>::value>::type>
{
   using type = be2_array<typename std::remove_all_extents<T>::type, std::extent<T>::value>;
};

/*
 * If Type is an class type, or union type, and NOT a cpu::Pointer<> type, then
 * we must dereference to be2_struct<Type>.
 */
template <typename T>
struct pointer_dereference_type<T,
   typename std::enable_if<(std::is_class<T>::value || std::is_union<T>::value)
                         && !is_cpu_pointer<T>::value
                         && !is_cpu_address<T>::value
                         && !is_cpu_func_pointer<T>::value
                         && !std::is_array<T>::value>::type>
{
   using type = be2_struct<T>;
};


/*
 * Basically the same as pointer_dereference_type but does not wrap structs in
 * be2_struct.
 *
 * Used for pointer_get_type::type * Pointer::get()
 */
template <typename T, typename = void>
struct pointer_get_type;

template <typename T>
struct pointer_get_type<T,
   typename std::enable_if<!(std::is_class<T>::value || std::is_union<T>::value)
                         || is_cpu_pointer<T>::value
                         || is_cpu_address<T>::value
                         || is_cpu_func_pointer<T>::value
                         || std::is_array<T>::value>::type>
{
   using type = typename pointer_dereference_type<T>::type;
};

template <typename T>
struct pointer_get_type<T,
   typename std::enable_if<(std::is_class<T>::value || std::is_union<T>::value)
                         && !is_cpu_pointer<T>::value
                         && !is_cpu_address<T>::value
                         && !is_cpu_func_pointer<T>::value
                         && !std::is_array<T>::value>::type>
{
   using type = T;
};

template<typename ValueType, typename AddressType>
class Pointer
{
public:
   using value_type = ValueType;
   using address_type = AddressType;
   using dereference_type = typename pointer_dereference_type<value_type>::type;
   using get_type = typename pointer_get_type<value_type>::type;

   static_assert(!std::is_pointer<value_type>::value,
                 "cpu::Pointer should point to another cpu::Pointer, not a raw T* pointer.");

   static_assert(!is_big_endian_value<value_type>::value,
                 "be2_val should not be used as ValueType for pointer, it is implied implicitly.");

   static_assert(std::is_class<value_type>::value
              || std::is_union<value_type>::value
              || std::is_arithmetic<value_type>::value
              || std::is_enum<value_type>::value
              || std::is_void<value_type>::value
              || std::is_array<value_type>::value,
                 "Invalid ValueType for Pointer");

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
   template<typename V = value_type>
   Pointer(const Pointer<typename std::remove_const<V>::type, address_type> &other,
           typename std::enable_if<std::is_const<V>::value>::type * = nullptr) :
      mAddress(other.mAddress)
   {
   }

   // Pointer<void>(Pointer<Type>)
   template<typename O, typename V = value_type>
   Pointer(const Pointer<O, address_type> &other,
           typename std::enable_if<std::is_void<V>::value>::type * = nullptr) :
      mAddress(other.mAddress)
   {
   }

   // Pointer<void>(be2_val<Pointer<Type>>)
   template<typename O, typename V = value_type>
   Pointer(const be2_val<Pointer<O, address_type>> &other,
           typename std::enable_if<std::is_void<V>::value>::type * = nullptr) :
      mAddress(other.value().mAddress)
   {
   }

   // Pointer<const Type> = const Pointer<Type> &
   template<typename V = value_type>
   typename std::enable_if<std::is_const<V>::value, Pointer>::type &
   operator =(const Pointer<typename std::remove_const<V>::type, address_type> &other)
   {
      mAddress = other.mAddress;
      return *this;
   }

   get_type *get() const
   {
      return internal::translate<dereference_type>(mAddress);
   }

   value_type *getRawPointer() const
   {
      return internal::translate<value_type>(mAddress);
   }

   explicit operator bool() const
   {
      return static_cast<bool>(mAddress);
   }

   explicit operator Pointer<void, address_type>() const
   {
      return { *this };
   }

   Pointer &
   operator =(std::nullptr_t)
   {
      mAddress = AddressType { 0u };
      return *this;
   }

   template<typename K = value_type>
   typename std::enable_if<!std::is_void<K>::value, dereference_type>::type &
   operator *()
   {
      return *internal::translate<dereference_type>(mAddress);
   }

   template<typename K = value_type>
   typename std::enable_if<!std::is_void<K>::value, const dereference_type>::type &
   operator *() const
   {
      return *internal::translate<const dereference_type>(mAddress);
   }

   template<typename K = value_type>
   typename std::enable_if<!std::is_void<K>::value, dereference_type>::type *
   operator ->()
   {
      return internal::translate<dereference_type>(mAddress);
   }

   template<typename K = value_type>
   typename std::enable_if<!std::is_void<K>::value, const dereference_type>::type *
   operator ->() const
   {
      return internal::translate<const dereference_type>(mAddress);
   }

   template<typename K = value_type>
   typename std::enable_if<!std::is_void<K>::value, dereference_type>::type &
   operator [](size_t index)
   {
      return internal::translate<dereference_type>(mAddress)[index];
   }

   template<typename K = value_type>
   typename std::enable_if<!std::is_void<K>::value, const dereference_type>::type &
   operator [](size_t index) const
   {
      return internal::translate<const dereference_type>(mAddress)[index];
   }

   constexpr bool operator == (std::nullptr_t) const
   {
      return !static_cast<bool>(mAddress);
   }

   template<typename O>
   constexpr bool operator == (const Pointer<O, address_type> &other) const
   {
      return mAddress == other.mAddress;
   }

   template<typename O>
   constexpr bool operator == (const be2_val<Pointer<O, address_type>> &other) const
   {
      return mAddress == other.value().mAddress;
   }

   constexpr bool operator != (std::nullptr_t) const
   {
      return static_cast<bool>(mAddress);
   }

   template<typename O>
   constexpr bool operator != (const Pointer<O, address_type> &other) const
   {
      return mAddress != other.mAddress;
   }

   template<typename O>
   constexpr bool operator != (const be2_val<Pointer<O, address_type>> &other) const
   {
      return mAddress != other.value().mAddress;
   }

   template<typename O>
   constexpr bool operator >= (const Pointer<O, address_type> &other) const
   {
      return mAddress >= other.mAddress;
   }

   template<typename O>
   constexpr bool operator >= (const be2_val<Pointer<O, address_type>> &other) const
   {
      return mAddress >= other.value().mAddress;
   }

   template<typename O>
   constexpr bool operator <= (const Pointer<O, address_type> &other) const
   {
      return mAddress <= other.mAddress;
   }

   template<typename O>
   constexpr bool operator <= (const be2_val<Pointer<O, address_type>> &other) const
   {
      return mAddress <= other.value().mAddress;
   }

   template<typename O>
   constexpr bool operator > (const Pointer<O, address_type> &other) const
   {
      return mAddress > other.mAddress;
   }

   template<typename O>
   constexpr bool operator > (const be2_val<Pointer<O, address_type>> &other) const
   {
      return mAddress > other.value().mAddress;
   }

   template<typename O>
   constexpr bool operator < (const Pointer<O, address_type> &other) const
   {
      return mAddress < other.mAddress;
   }

   template<typename O>
   constexpr bool operator < (const be2_val<Pointer<O, address_type>> &other) const
   {
      return mAddress < other.value().mAddress;
   }

   template<typename T = value_type>
   constexpr typename std::enable_if<!std::is_void<T>::value, Pointer &>::type
   operator ++ ()
   {
      mAddress += sizeof(value_type);
      return *this;
   }

   template<typename T = value_type>
   constexpr typename std::enable_if<!std::is_void<T>::value, Pointer>::type
   operator ++ (int)
   {
      auto prev = *this;
      mAddress += sizeof(value_type);
      return prev;
   }

   template<typename T = value_type>
   constexpr typename std::enable_if<!std::is_void<T>::value, Pointer &>::type
   operator += (ptrdiff_t value)
   {
      mAddress += value * sizeof(value_type);
      return *this;
   }

   template<typename T = value_type>
   constexpr typename std::enable_if<!std::is_void<T>::value, Pointer &>::type
   operator -= (ptrdiff_t value)
   {
      mAddress -= value * sizeof(value_type);
      return *this;
   }

   template<typename T = value_type>
   constexpr typename std::enable_if<!std::is_void<T>::value, Pointer>::type
   operator + (ptrdiff_t value) const
   {
      Pointer dst;
      dst.mAddress = mAddress + (value * sizeof(value_type));
      return dst;
   }

   template<typename T = value_type>
   constexpr typename std::enable_if<!std::is_void<T>::value, ptrdiff_t>::type
   operator -(const Pointer &other) const
   {
      return (mAddress - other.mAddress) / sizeof(value_type);
   }

   template<typename T = value_type>
   constexpr typename std::enable_if<!std::is_void<T>::value, Pointer>::type
   operator -(ptrdiff_t value) const
   {
      Pointer dst;
      dst.mAddress = mAddress - (value * sizeof(value_type));
      return dst;
   }

protected:
   template<typename, typename, typename, typename>
   friend struct pointer_cast_impl;

   template<typename AddressType2, typename ValueType2>
   friend class Pointer;

   address_type mAddress;
};

template<typename Value>
using VirtualPointer = Pointer<Value, VirtualAddress>;

template<typename Value>
using PhysicalPointer = Pointer<Value, PhysicalAddress>;

template<typename AddressType, typename SrcTypePtr, typename DstTypePtr>
struct pointer_cast_impl<AddressType, SrcTypePtr, DstTypePtr,
   typename std::enable_if<std::is_pointer<SrcTypePtr>::value && std::is_pointer<DstTypePtr>::value>::type>
{
   using DstType = typename std::remove_pointer<DstTypePtr>::type;
   using SrcType = typename std::remove_pointer<SrcTypePtr>::type;

   // Pointer<X, AddressType> to Pointer<Y, AddressType>
   static constexpr Pointer<DstType, AddressType> cast(Pointer<SrcType, AddressType> src)
   {
      Pointer<DstType, AddressType> dst;
      dst.mAddress = src.mAddress;
      return dst;
   }
};

template<typename AddressType, typename DstTypePtr>
struct pointer_cast_impl<AddressType, AddressType, DstTypePtr,
   typename std::enable_if<std::is_pointer<DstTypePtr>::value>::type>
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

template<typename AddressType, typename SrcTypePtr>
struct pointer_cast_impl<AddressType, SrcTypePtr, AddressType,
   typename std::enable_if<std::is_pointer<SrcTypePtr>::value>::type>
{
   using SrcType = typename std::remove_pointer<SrcTypePtr>::type;

   // Pointer<X, AddressType> to AddressType
   static constexpr AddressType cast(Pointer<SrcType, AddressType> src)
   {
      return src.mAddress;
   }
};

} // namespace cpu

// Custom formatters for fmtlib
namespace fmt
{

inline namespace v5
{
template<typename Type, typename Char, typename Enabled>
struct formatter;
}

template<typename OutputIt>
auto format_escaped_string(OutputIt iter, const char *data)
{
   iter = format_to(iter, "\"");

   auto hasMoreBytes = true;
   for (auto i = 0; i < 128; ++i) {
      auto c = data[i];
      if (c == 0) {
         hasMoreBytes = false;
         break;
      }

      if (c >= ' ' && c <= '~' && c != '\\' && c != '"') {
         iter = format_to(iter, "{}", c);
      } else {
         switch (c) {
         case '"': iter = format_to(iter, "\\\""); break;
         case '\\': iter = format_to(iter, "\\\\"); break;
         case '\t': iter = format_to(iter, "\\t"); break;
         case '\r': iter = format_to(iter, "\\r"); break;
         case '\n': iter = format_to(iter, "\\n"); break;
         default: iter = format_to(iter, "\\x{:02x}", c); break;
         }
      }
   }

   if (!hasMoreBytes) {
      iter = format_to(iter, "\"");
   } else {
      iter = format_to(iter, "\"...");
   }

   return iter;
}

template<typename AddressType, typename Char>
struct formatter<cpu::Pointer<char, AddressType>, Char, void>
{
   template<typename ParseContext>
   constexpr auto parse(ParseContext &ctx)
   {
      return ctx.begin();
   }

   template<typename FormatContext>
   auto format(const cpu::Pointer<char, AddressType> &ptr, FormatContext &ctx)
   {
      if (!ptr) {
         return format_to(ctx.begin(), "<NULL>");
      } else {
         auto bytes = ptr.getRawPointer();
         return format_escaped_string(ctx.begin(), bytes);
      }
   }
};

template<typename AddressType, typename Char>
struct formatter<cpu::Pointer<const char, AddressType>, Char, void>
{
   template<typename ParseContext>
   constexpr auto parse(ParseContext &ctx)
   {
      return ctx.begin();
   }

   template<typename FormatContext>
   auto format(const cpu::Pointer<const char, AddressType> &ptr, FormatContext &ctx)
   {
      if (!ptr) {
         return format_to(ctx.begin(), "<NULL>");
      } else {
         const char *bytes = ptr.getRawPointer();
         return format_escaped_string(ctx.begin(), bytes);
      }
   }
};

template<typename ValueType, typename AddressType, typename Char>
struct formatter<cpu::Pointer<ValueType, AddressType>, Char, void>
{
   template<typename ParseContext>
   constexpr auto parse(ParseContext &ctx)
   {
      return ctx.begin();
   }

   template<typename FormatContext>
   auto format(const cpu::Pointer<ValueType, AddressType> &ptr, FormatContext &ctx)
   {
      auto addr = cpu::pointer_cast_impl<AddressType, ValueType *, AddressType>::cast(ptr);
      return format_to(ctx.begin(), "0x{:08X}", static_cast<uint32_t>(addr));
   }
};

} // namespace fmt

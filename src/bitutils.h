#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <type_traits>

// reinterpret_cast for value types
template<typename DstType, typename SrcType>
static inline DstType
bit_cast(const SrcType& src)
{
   static_assert(sizeof(SrcType) == sizeof(DstType), "bit_cast must be between same sized types");
   static_assert(std::is_trivially_copyable<SrcType>(), "SrcType is not trivially copyable.");
   static_assert(std::is_trivially_copyable<DstType>(), "DstType is not trivially copyable.");

   DstType dst;
   std::memcpy(&dst, &src, sizeof(SrcType));
   return dst;
}

// Gets the value of a bit
template<typename Type>
inline Type
get_bit(Type src, unsigned bit)
{
   return (src >> bit) & static_cast<Type>(1);
}

template<unsigned bit, typename Type>
inline Type
get_bit(Type src)
{
   return (src >> (bit)) & static_cast<Type>(1);
}

// Sets the value of a bit to 1
template<typename Type>
inline Type
set_bit(Type src, unsigned bit)
{
   return src | (static_cast<Type>(1) << bit);
}

template<unsigned bit, typename Type>
inline Type
set_bit(Type src)
{
   return src | (static_cast<Type>(1) << (bit));
}

// Clears the value of a bit
template<typename Type>
inline Type
clear_bit(Type src, unsigned bit)
{
   return src & ~(static_cast<Type>(1) << bit);
}

template<unsigned bit, typename Type>
inline Type
clear_bit(Type src)
{
   return src & ~(static_cast<Type>(1) << (bit));
}

// Sets the value of a bit to value
template<typename Type>
inline Type
set_bit_value(Type src, unsigned bit, Type value)
{
   src = clear_bit(src, bit);
   return src | ((value & static_cast<Type>(1)) << bit);
}

template<unsigned bit, typename Type>
inline Type
set_bit_value(Type src, Type value)
{
   src = clear_bit(src, bit);
   return src | (value << bit);
}

// Create a bitmask for bits
template<typename Type>
constexpr inline Type
make_bitmask(Type bits)
{
   return static_cast<Type>((1ull << bits) - 1);
}

template<unsigned bits, typename Type>
constexpr inline Type
make_bitmask()
{
   return static_cast<Type>((1ull << (bits)) - 1);
}

template<>
constexpr inline uint32_t
make_bitmask<32, uint32_t>()
{
   return 0xffffffff;
}

template<>
constexpr inline uint64_t
make_bitmask<64, uint64_t>()
{
   return 0xffffffffffffffffull;
}

// Creates a bitmask between begin and end
template<typename Type>
constexpr inline Type
make_bitmask(Type begin, Type end)
{
   return make_bitmask(end - begin + 1) << begin;
}

template<unsigned begin, unsigned end, typename Type>
constexpr inline Type
make_bitmask()
{
   return make_bitmask<(end) - (begin) + 1, Type>() << (begin);
}

// Creates a bitmask between mb and me
inline uint32_t
make_ppc_bitmask(int mb, int me)
{
   uint32_t begin, end, mask;
   begin = 0xFFFFFFFF >> mb;
   end = me < 31 ? (0xFFFFFFFF >> (me + 1)) : 0;
   mask = begin ^ end;
   return (me < mb) ? ~mask : mask;
}

// Sign extend bits to int32_t
template<typename Type>
inline Type
sign_extend(Type src, unsigned bits)
{
   auto mask = make_bitmask<Type>(bits);
   src &= mask;

   if (get_bit(src, bits)) {
      return src | ~mask;
   } else {
      return src;
   }
}

template<unsigned bits, typename Type>
inline Type
sign_extend(Type src)
{
   auto mask = make_bitmask<bits, Type>();
   src &= mask;

   if (get_bit<(bits) - 1>(src)) {
      return src | ~mask;
   } else {
      return src;
   }
}

// Return number of bits in type
template<typename Type>
struct bit_width
{
   static const size_t value = sizeof(Type) * 8;

   operator size_t() const
   {
      return value;
   }
};

// Utility class to swap endian for types of size 1, 2, 4, 8
// other type sizes are not supported
template<typename Type, unsigned Size = sizeof(Type)>
struct byte_swap_t;

template<typename Type>
struct byte_swap_t<Type, 1>
{
   static Type swap(Type src)
   {
      return src;
   }
};

template<typename Type>
struct byte_swap_t<Type, 2>
{
   static Type swap(Type src)
   {
      return bit_cast<Type>(_byteswap_ushort(bit_cast<uint16_t>(src)));
   }
};

template<typename Type>
struct byte_swap_t<Type, 4>
{
   static Type swap(Type src)
   {
      return bit_cast<Type>(_byteswap_ulong(bit_cast<uint32_t>(src)));
   }
};

template<typename Type>
struct byte_swap_t<Type, 8>
{
   static Type swap(Type src)
   {
      return bit_cast<Type>(_byteswap_uint64(bit_cast<uint64_t>(src)));
   }
};

// Swaps endian of src
template<typename Type>
inline Type
byte_swap(Type src)
{
   return byte_swap_t<Type>::swap(src);
}

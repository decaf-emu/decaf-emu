#pragma once
#include "platform.h"
#include <climits>
#include <cstdint>
#include <cstddef>

#ifdef PLATFORM_WINDOWS
#include <intrin.h>
#endif

// Gets the value of a bit
template<typename Type>
constexpr Type
get_bit(Type src, unsigned bit)
{
   return (src >> bit) & static_cast<Type>(1);
}

template<unsigned bit, typename Type>
constexpr Type
get_bit(Type src)
{
   return (src >> (bit)) & static_cast<Type>(1);
}

// Sets the value of a bit to 1
template<typename Type>
constexpr Type
set_bit(Type src, unsigned bit)
{
   return src | (static_cast<Type>(1) << bit);
}

template<unsigned bit, typename Type>
constexpr Type
set_bit(Type src)
{
   return src | (static_cast<Type>(1) << (bit));
}

// Flips the value of a bit to 1
template<typename Type>
constexpr Type
flip_bit(Type src, unsigned bit)
{
   return src ^ (static_cast<Type>(1) << bit);
}

template<unsigned bit, typename Type>
constexpr Type
flip_bit(Type src)
{
   return src ^ (static_cast<Type>(1) << (bit));
}

// Clears the value of a bit
template<typename Type>
constexpr Type
clear_bit(Type src, unsigned bit)
{
   return src & ~(static_cast<Type>(1) << bit);
}

template<unsigned bit, typename Type>
constexpr Type
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
constexpr Type
make_bitmask(Type bits)
{
   return static_cast<Type>((1ull << bits) - 1);
}

template<unsigned bits, typename Type>
constexpr Type
make_bitmask()
{
   return static_cast<Type>((1ull << (bits)) - 1);
}

template<>
constexpr uint32_t
make_bitmask<32, uint32_t>()
{
   return 0xffffffff;
}

template<>
constexpr uint64_t
make_bitmask<64, uint64_t>()
{
   return 0xffffffffffffffffull;
}

// Creates a bitmask between begin and end
template<typename Type>
constexpr Type
make_bitmask(Type begin, Type end)
{
   return make_bitmask(end - begin + 1) << begin;
}

template<unsigned begin, unsigned end, typename Type>
constexpr Type
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

#ifdef PLATFORM_WINDOWS
inline int
clz(uint32_t bits)
{
   unsigned long a;
   if (!_BitScanReverse(&a, bits)) {
      return 32;
   } else {
      return 31 - a;
   }
}
#else
#define clz __builtin_clz
#endif

#ifdef PLATFORM_WINDOWS
inline int
clz64(uint64_t bits)
{
   unsigned long a;
   if (!_BitScanReverse64(&a, bits)) {
      return 64;
   } else {
      return 63 - a;
   }
}
#else
#define clz64 __builtin_clzll
#endif

inline bool
bit_scan_reverse(unsigned long *out_position, uint32_t bits)
{
#ifdef PLATFORM_WINDOWS
   return !!_BitScanReverse(out_position, bits);
#elif defined(PLATFORM_POSIX)
   if (bits == 0) {
      return false;
   }

   *out_position = 31 - __builtin_clz(bits);
   return true;
#endif
}

#ifdef PLATFORM_WINDOWS
#define bit_rotate_left _rotl
#else
inline uint32_t bit_rotate_left(uint32_t x, int shift)
{
   shift &= 31;

   if (!shift) {
     return x;
   }

   return (x << shift) | (x >> (32 - shift));
}
#endif

#ifdef PLATFORM_WINDOWS
#define bit_rotate_right _rotr
#else
inline uint32_t bit_rotate_right(uint32_t x, int shift)
{
   shift &= 31;

   if (!shift) {
      return x;
   }

   return (x >> shift) | (x << (32 - shift));
}
#endif

// Return number of bits in type
template<typename Type>
struct bit_width
{
   static constexpr size_t value = sizeof(Type) * CHAR_BIT;

   constexpr operator size_t() const
   {
      return value;
   }
};

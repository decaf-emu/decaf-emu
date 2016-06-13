#pragma once
#include "platform.h"
#include "bit_cast.h"
#include <cstdint>

#ifdef PLATFORM_LINUX
#include <byteswap.h>
#endif

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
#ifdef PLATFORM_WINDOWS
      return bit_cast<Type>(_byteswap_ushort(bit_cast<uint16_t>(src)));
#elif defined(PLATFORM_APPLE)
      // Apple has no 16-bit byteswap intrinsic
      const uint16_t data = bit_cast<uint16_t>(src);
      return bit_cast<Type>((uint16_t)((data >> 8) | (data << 8)));
#elif defined(PLATFORM_LINUX)
      return bit_cast<Type>(bswap_16(bit_cast<uint16_t>(src)));
#endif
   }
};

template<typename Type>
struct byte_swap_t<Type, 4>
{
   static Type swap(Type src)
   {
#ifdef PLATFORM_WINDOWS
      return bit_cast<Type>(_byteswap_ulong(bit_cast<uint32_t>(src)));
#elif defined(PLATFORM_APPLE)
      return bit_cast<Type>(__builtin_bswap32(bit_cast<uint32_t>(src)));
#elif defined(PLATFORM_LINUX)
      return bit_cast<Type>(bswap_32(bit_cast<uint32_t>(src)));
#endif
   }
};

template<typename Type>
struct byte_swap_t<Type, 8>
{
   static Type swap(Type src)
   {
#ifdef PLATFORM_WINDOWS
      return bit_cast<Type>(_byteswap_uint64(bit_cast<uint64_t>(src)));
#elif defined(PLATFORM_APPLE)
      return bit_cast<Type>(__builtin_bswap64(bit_cast<uint64_t>(src)));
#elif defined(PLATFORM_LINUX)
      return bit_cast<Type>(bswap_64(bit_cast<uint64_t>(src)));
#endif
   }
};

// Swaps endian of src
template<typename Type>
inline Type
byte_swap(Type src)
{
   return byte_swap_t<Type>::swap(src);
}

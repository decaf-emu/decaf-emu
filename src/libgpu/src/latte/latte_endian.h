#pragma once
#include "latte/latte_enum_cb.h"
#include <common/byte_swap.h>

namespace latte
{

static inline uint64_t
applyEndianSwap(uint64_t value, latte::CB_ENDIAN swap)
{
   static const auto swap16 = [](uint64_t value, int pos)
      {
         auto word = static_cast<uint16_t>((value >> pos) & 0xFFFFu);
         word = byte_swap(word);
         return static_cast<uint64_t>(word) << pos;
      };
   static const auto swap32 = [](uint64_t value, int pos)
      {
         auto word = static_cast<uint32_t>((value >> pos) & 0xFFFFFFFFu);
         word = byte_swap(word);
         return static_cast<uint64_t>(word) << pos;
      };

   switch (swap) {
   case latte::CB_ENDIAN::NONE:
      break;
   case latte::CB_ENDIAN::SWAP_8IN16:
      value = swap16(value, 0) | swap16(value, 16) |
              swap16(value, 32) | swap16(value, 48);
      break;
   case latte::CB_ENDIAN::SWAP_8IN32:
      value = swap32(value, 0) | swap32(value, 32);
      break;
   case latte::CB_ENDIAN::SWAP_8IN64:
      value = byte_swap(value);
      break;
   }

   return value;
}

} // namespace latte

#pragma once

#include <cstdint>

union Instruction
{
   Instruction() {}
   constexpr Instruction(uint32_t value) : value(value) {}

   uint32_t value;

   operator uint32_t() const
   {
      return value;
   }

#define FLD(x, y, z, ...) \
   struct { \
      uint32_t : (31-z); \
      uint32_t x : (z-y+1); \
      uint32_t : (y); \
   };
#define MRKR(...)
#include "instructionfields.inl"
#undef FLD
#undef MRKR

};

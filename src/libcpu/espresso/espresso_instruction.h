#pragma once
#include <cstdint>

namespace espresso
{

union Instruction
{
   Instruction() : value(0)
   {
   }

   constexpr Instruction(uint32_t value_) : value(value_)
   {
   }

   uint32_t value;

   operator uint32_t() const
   {
      return value;
   }

#define FLD(x, y, z, ...) \
   struct { \
      uint32_t : (31 - z); \
      uint32_t x : (z - y + 1); \
      uint32_t : (y); \
   };
#define MRKR(...)
#include "espresso_instruction_fields.inl"
#undef FLD
#undef MRKR
};

} // namespace espresso

#pragma once
#include "latte_registers.h"
#include <gsl.h>

namespace latte
{

namespace pm4
{

class PacketSizer
{
public:
   PacketSizer() :
      mPayloadSize(0)
   {
   }

   // Read one uint32_t sized datatype
   template <typename Type>
   PacketSizer &operator()(Type value)
   {
      static_assert(sizeof(Type) == sizeof(uint32_t), "Invalid type size");
      mPayloadSize++;
      return *this;
   }

   // Write a pointer as one word
   template<typename Type>
   PacketSizer &operator()(Type *value)
   {
      mPayloadSize++;
      return *this;
   }

   // Write a list of words
   template<typename Type>
   PacketSizer &operator()(const gsl::span<Type> &values)
   {
      auto dataSize = gsl::narrow_cast<uint32_t>(((values.size() * sizeof(Type)) + 3) / 4);
      mPayloadSize += dataSize;
      return *this;
   }

   // Write one word as a REG_OFFSET
   PacketSizer &REG_OFFSET(latte::Register value, latte::Register base)
   {
      mPayloadSize++;
      return *this;
   }

   // Write one word as a CONST_OFFSET
   PacketSizer &CONST_OFFSET(uint32_t value)
   {
      mPayloadSize++;
      return *this;
   }

   // Write one word as a size (N - 1)
   template<typename Type>
   PacketSizer &size(Type value)
   {
      mPayloadSize++;
      return *this;
   }

   uint32_t getSize() const
   {
      return mPayloadSize;
   }

protected:
   uint32_t mPayloadSize;
};

} // namespace pm4

} // namespace latte

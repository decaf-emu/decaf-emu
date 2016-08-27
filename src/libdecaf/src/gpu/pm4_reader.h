#pragma once
#include "pm4_buffer.h"
#include "pm4_format.h"
#include "virtual_ptr.h"
#include <common/decaf_assert.h>
#include <gsl.h>

namespace pm4
{

/**
 * Note: packet reader assumes the buffer has already been byte swapped to
 * little endian before reading. This is because it returns pointers to the
 * buffer, and we have to be able to return swapped memory. We cannot do an
 * in place swap because that could modify the game's memory.
 */
class PacketReader
{
public:
   PacketReader(gsl::span<uint32_t> data) :
      mBuffer(data)
   {
   }

   // Read one word
   PacketReader &operator()(uint32_t &value)
   {
      checkSize(1);
      value = mBuffer[mPosition++];
      return *this;
   }

   // Read one float
   PacketReader &operator()(float &value)
   {
      checkSize(1);
      value = bit_cast<float>(mBuffer[mPosition++]);
      return *this;
   }

   // Read one uint32_t sized datatype
   template <typename Type>
   PacketReader &operator()(Type &value)
   {
      static_assert(sizeof(Type) == sizeof(uint32_t), "Invalid type size");
      checkSize(1);
      value = bit_cast<Type>(mBuffer[mPosition++]);
      return *this;
   }

   // Read one word as a virtual_ptr
   template<typename Type>
   PacketReader &operator()(virtual_ptr<Type> &value)
   {
      checkSize(1);
      value.setAddress(mBuffer[mPosition++]);
      return *this;
   }

   // Read the rest of the entire packet
   template<typename Type>
   PacketReader &operator()(gsl::span<Type> &values)
   {
      values = gsl::as_span(reinterpret_cast<Type*>(&mBuffer[mPosition]),
                            ((mBuffer.size() - mPosition) * sizeof(uint32_t)) / sizeof(Type));

      mPosition = mBuffer.size();
      return *this;
   }

   // Read one word as a REG_OFFSET
   PacketReader &REG_OFFSET(latte::Register &value, latte::Register base)
   {
      checkSize(1);
      value = static_cast<latte::Register>(((mBuffer[mPosition++] & 0xFFFF) * 4) + (uint32_t)base);
      return *this;
   }

   // Read one word as a CONST_OFFSET
   PacketReader &CONST_OFFSET(uint32_t &value)
   {
      checkSize(1);
      value = mBuffer[mPosition++] & 0xFFFF;
      return *this;
   }

   // Read one word as a size (N - 1)
   template<typename Type>
   PacketReader &size(Type &value)
   {
      checkSize(1);
      value = static_cast<Type>(mBuffer[mPosition++] + 1);
      return *this;
   }

private:
   void checkSize(size_t size)
   {
      if (mPosition + size > mBuffer.size()) {
         decaf_abort("Read past end of packet");
      }
   }

private:
   size_t mPosition = 0;
   gsl::span<uint32_t> mBuffer;
};

template<typename Type>
Type read(PacketReader &reader)
{
   Type result;
   result.serialise(reader);
   return result;
}

}

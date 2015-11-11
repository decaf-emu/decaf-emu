#pragma once
#include <array_view.h>
#include "pm4_buffer.h"
#include "pm4_format.h"
#include "utils/virtual_ptr.h"

namespace pm4
{

class PacketReader
{
public:
   PacketReader(gsl::array_view<uint32_t> data) :
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

   // Read one word as a virtual_ptr
   template<typename Type>
   PacketReader &operator()(virtual_ptr<Type> &value)
   {
      checkSize(1);
      value.setAddress(mBuffer[mPosition++]);
      return *this;
   }

   // Read the rest of the entire packet
   PacketReader &operator()(gsl::array_view<uint32_t> &values)
   {
      values = mBuffer.sub(mPosition);
      mPosition = mBuffer.size();
      return *this;
   }

   // Read array of size
   PacketReader &operator()(gsl::array_view<uint32_t> &values, size_t size)
   {
      checkSize(size);
      values = mBuffer.sub(mPosition, size);
      mPosition += size;
      return *this;
   }

   // Read one word as a register
   template<typename Type>
   PacketReader &reg(Type &value, uint32_t base)
   {
      checkSize(1);
      value = static_cast<Type>((mBuffer[mPosition++] << 2) + base);
      return *this;
   }

private:
   void checkSize(size_t size)
   {
      if (mPosition + size > mBuffer.size()) {
         throw std::logic_error("Read past end of packet");
      }
   }

private:
   size_t mPosition = 0;
   gsl::array_view<uint32_t> mBuffer;
};

template<typename Type>
Type read(PacketReader &reader)
{
   Type result;
   result.serialise(reader);
   return result;
}

}

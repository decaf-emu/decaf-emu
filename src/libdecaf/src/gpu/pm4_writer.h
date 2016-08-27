#pragma once
#include "pm4_buffer.h"
#include "pm4_format.h"
#include "pm4_packets.h"
#include "latte_registers.h"
#include "virtual_ptr.h"
#include <common/decaf_assert.h>
#include <common/log.h>
#include <gsl.h>

namespace pm4
{

class PacketSizer
{
public:
   PacketSizer()
      : mPayloadSize(0)
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

   // Write a virtual ptr as one word
   template<typename Type>
   PacketSizer &operator()(virtual_ptr<Type> value)
   {
      mPayloadSize++;
      return *this;
   }

   // Write a list of words
   template<typename Type>
   PacketSizer &operator()(const gsl::span<Type> &values)
   {
      auto size = gsl::narrow_cast<uint32_t>(((values.size() * sizeof(Type)) + 3) / 4);
      mPayloadSize += size;
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

class PacketWriter
{
public:
   PacketWriter(type3::IT_OPCODE op, uint32_t payloadSize)
   {
      mBuffer = pm4::getBuffer(payloadSize + 1);

      mTotalSize = payloadSize + 1;
      mSaveSize = mBuffer->curSize;

      auto header = type3::Header::get(0)
         .type(Header::Type3)
         .opcode(op)
         .size(mTotalSize - 2);

      mBuffer->buffer[mBuffer->curSize++] = byte_swap(header.value);
   }

   ~PacketWriter()
   {
      decaf_check(mBuffer->curSize - mSaveSize == mTotalSize);
   }

   // Write one word
   PacketWriter &operator()(uint32_t value)
   {
      mBuffer->buffer[mBuffer->curSize++] = byte_swap(value);
      return *this;
   }

   // Write one float
   PacketWriter &operator()(float value)
   {
      mBuffer->buffer[mBuffer->curSize++] = byte_swap(bit_cast<uint32_t>(value));
      return *this;
   }

   // Read one uint32_t sized datatype
   template <typename Type>
   PacketWriter &operator()(Type value)
   {
      static_assert(sizeof(Type) == sizeof(uint32_t), "Invalid type size");
      mBuffer->buffer[mBuffer->curSize++] = byte_swap(bit_cast<uint32_t>(value));
      return *this;
   }

   // Write a virtual ptr as one word
   template<typename Type>
   PacketWriter &operator()(virtual_ptr<Type> value)
   {
      mBuffer->buffer[mBuffer->curSize++] = byte_swap(value.getAddress());
      return *this;
   }

   // Write a list of words
   template<typename Type>
   PacketWriter &operator()(const gsl::span<Type> &values)
   {
      auto size = gsl::narrow_cast<uint32_t>(((values.size() * sizeof(Type)) + 3) / 4);
      std::memcpy(&mBuffer->buffer[mBuffer->curSize], values.data(), size * sizeof(uint32_t));

      // We do the byte_swap here separately as Type may not be uint32_t sized
      for (auto i = 0u; i < size; ++i) {
         mBuffer->buffer[mBuffer->curSize + i] = byte_swap(mBuffer->buffer[mBuffer->curSize + i]);
      }

      mBuffer->curSize += size;
      return *this;
   }

   // Write one word as a REG_OFFSET
   PacketWriter &REG_OFFSET(latte::Register value, latte::Register base)
   {
      auto offset = static_cast<uint32_t>(value) - static_cast<uint32_t>(base);
      mBuffer->buffer[mBuffer->curSize++] = byte_swap(offset / 4);
      return *this;
   }

   // Write one word as a CONST_OFFSET
   PacketWriter &CONST_OFFSET(uint32_t value)
   {
      mBuffer->buffer[mBuffer->curSize++] = byte_swap(value);
      return *this;
   }

   // Write one word as a size (N - 1)
   template<typename Type>
   PacketWriter &size(Type value)
   {
      mBuffer->buffer[mBuffer->curSize++] = byte_swap(static_cast<uint32_t>(value) - 1);
      return *this;
   }

private:
   pm4::Buffer *mBuffer;
   uint32_t mTotalSize;
   uint32_t mSaveSize;
};

template<typename Type>
void write(const Type &value)
{
   auto &ncValue = const_cast<Type&>(value);

   // Calculate the total size this object will be
   PacketSizer sizer;
   ncValue.serialise(sizer);

   // Serialize the packet to the active command buffer
   PacketWriter writer(Type::Opcode, sizer.getSize());
   ncValue.serialise(writer);
}

} // namespace pm4

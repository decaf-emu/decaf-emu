#pragma once
#include <array_view.h>
#include <gsl.h>
#include "pm4_buffer.h"
#include "pm4_format.h"
#include "utils/virtual_ptr.h"

namespace pm4
{

class PacketWriter
{
public:
   PacketWriter(Opcode3::Value opCode)
   {
      mBuffer = pm4::getBuffer(1);
      mSaveSize = mBuffer->curSize;

      auto header = reinterpret_cast<Packet3 *>(&mBuffer->buffer[mBuffer->curSize++]);
      header->type = PacketType::Type3;
      header->opcode = opCode;
      header->size = 0;
   }

   ~PacketWriter()
   {
      auto header = reinterpret_cast<Packet3 *>(&mBuffer->buffer[mSaveSize]);
      header->size = mBuffer->curSize - mSaveSize;
   }

   // Write one word
   PacketWriter &operator()(uint32_t value)
   {
      checkSize(1);
      mBuffer->buffer[mBuffer->curSize++] = value;
      return *this;
   }

   // Write one word with a base value
   template<typename Type>
   PacketWriter &offsetValue(Type value, Type base)
   {
      checkSize(1);
      mBuffer->buffer[mBuffer->curSize++] = static_cast<uint32_t>(value) - static_cast<uint32_t>(base);
      return *this;
   }

   // Write a virtual ptr as one word
   template<typename Type>
   PacketWriter &operator()(virtual_ptr<Type> value)
   {
      checkSize(1);
      mBuffer->buffer[mBuffer->curSize++] = value.getAddress();
      return *this;
   }

   // Write a list of words
   PacketWriter &operator()(gsl::array_view<uint32_t> values)
   {
      auto size = gsl::narrow_cast<uint32_t>(values.size());
      checkSize(size);
      memcpy(&mBuffer->buffer[mBuffer->curSize], values.data(), values.size() * sizeof(uint32_t));
      mBuffer->curSize += size;
      return *this;
   }

private:
   void checkSize(uint32_t size)
   {
      if (mBuffer->curSize + size <= mBuffer->maxSize) {
         return;
      }

      // Reset old buffer
      auto oldBuffer = mBuffer;
      auto oldSize = oldBuffer->curSize;
      oldBuffer->curSize = mSaveSize;

      // Copy to new buffer
      auto newBuffer = pm4::flushBuffer(oldBuffer);
      auto saveSize = newBuffer->curSize;

      if (oldSize > mSaveSize) {
         memcpy(&newBuffer->buffer[newBuffer->curSize], &oldBuffer->buffer[mSaveSize], oldSize - mSaveSize);
         newBuffer->curSize = oldBuffer->curSize - mSaveSize;
      }

      mBuffer->curSize = mSaveSize;
      mBuffer = newBuffer;
      mSaveSize = saveSize;
   }

private:
   pm4::Buffer *mBuffer;
   uint32_t mSaveSize;
};

template<typename Type>
void write(Type &value)
{
   PacketWriter writer { Type::Opcode };
   value.serialise(writer);
}

}
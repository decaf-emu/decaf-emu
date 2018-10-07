#pragma once
#include "latte_registers.h"
#include "latte_pm4_commands.h"

#include <libcpu/mmu.h>
#include <gsl.h>

namespace latte::pm4
{

class PacketWriter
{
public:
   PacketWriter(uint32_t *buffer,
                uint32_t &outSize,
                IT_OPCODE op,
                uint32_t totalSize) :
      mBuffer(buffer),
      mCurSize(outSize)
   {
      mTotalSize = totalSize;
      mSaveSize = mCurSize;

      auto header = latte::pm4::HeaderType3::get(0)
         .type(latte::pm4::PacketType::Type3)
         .opcode(op)
         .size(mTotalSize - 2);

      mBuffer[mCurSize++] = byte_swap(header.value);
   }

   ~PacketWriter()
   {
      decaf_check(mCurSize - mSaveSize == mTotalSize);
   }

   // Write one word
   PacketWriter &operator()(uint32_t value)
   {
      mBuffer[mCurSize++] = byte_swap(value);
      return *this;
   }

   // Write one float
   PacketWriter &operator()(float value)
   {
      mBuffer[mCurSize++] = byte_swap(bit_cast<uint32_t>(value));
      return *this;
   }

   // Read one uint32_t sized datatype
   template <typename Type>
   PacketWriter &operator()(Type value)
   {
      static_assert(sizeof(Type) == sizeof(uint32_t), "Invalid type size");
      mBuffer[mCurSize++] = byte_swap(bit_cast<uint32_t>(value));
      return *this;
   }

   // Write a list of words
   template<typename Type>
   PacketWriter &operator()(const gsl::span<Type> &values)
   {
      auto dataSize = gsl::narrow_cast<uint32_t>(((values.size() * sizeof(Type)) + 3) / 4);
      std::memcpy(mBuffer + mCurSize, values.data(), dataSize * sizeof(uint32_t));

      // We do the byte_swap here separately as Type may not be uint32_t sized
      for (auto i = 0u; i < dataSize; ++i) {
         mBuffer[mCurSize + i] = byte_swap(mBuffer[mCurSize + i]);
      }

      mCurSize += dataSize;
      return *this;
   }

   // Write a list of already swapped words
   template<typename Type>
   PacketWriter &operator()(const gsl::span<be2_val<Type>> &values)
   {
      auto dataSize = gsl::narrow_cast<uint32_t>(((values.size() * sizeof(Type)) + 3) / 4);
      std::memcpy(mBuffer + mCurSize, values.data(), dataSize * sizeof(uint32_t));
      mCurSize += dataSize;
      return *this;
   }

   // Write one word as a REG_OFFSET
   PacketWriter &REG_OFFSET(latte::Register value, latte::Register base)
   {
      auto offset = static_cast<uint32_t>(value) - static_cast<uint32_t>(base);
      mBuffer[mCurSize++] = byte_swap(offset / 4);
      return *this;
   }

   // Write one word as a CONST_OFFSET
   PacketWriter &CONST_OFFSET(uint32_t value)
   {
      mBuffer[mCurSize++] = byte_swap(value);
      return *this;
   }

   // Write one word as a size (N - 1)
   template<typename Type>
   PacketWriter &size(Type value)
   {
      mBuffer[mCurSize++] = byte_swap(static_cast<uint32_t>(value) - 1);
      return *this;
   }

private:
   uint32_t *mBuffer;
   uint32_t &mCurSize;

   uint32_t mTotalSize;
   uint32_t mSaveSize;
};

} // namespace namespace latte::pm4

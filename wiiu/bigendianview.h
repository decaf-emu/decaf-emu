#pragma once
#include "byte_swap.h"

class BigEndianView
{
public:
   BigEndianView(const char *buffer, size_t size) :
      mBuffer(buffer), mSize(size), mOffset(0)
   {
   }

   template<typename Type>
   void read(Type &value)
   {
      value = byte_swap(*reinterpret_cast<const Type*>(mBuffer + mOffset));
      mOffset += sizeof(Type);
   }

   template<typename Type>
   Type read()
   {
      Type value = byte_swap(*reinterpret_cast<const Type*>(mBuffer + mOffset));
      mOffset += sizeof(Type);
      return value;
   }

   template<typename Type, size_t count>
   void read(Type(&chars)[count])
   {
      memcpy(chars, mBuffer + mOffset, sizeof(Type) * count);
      mOffset += sizeof(Type) * count;
   }

   template<typename Type>
   void read(Type *buffer, size_t count)
   {
      memcpy(buffer, mBuffer + mOffset, sizeof(Type) * count);
      mOffset += sizeof(Type) * count;
   }

   template<typename Type>
   const Type *readRaw(size_t count)
   {
      auto ptr = reinterpret_cast<const Type*>(mBuffer + mOffset);
      mOffset += sizeof(Type) * count;
      return ptr;
   }

   void seek(size_t pos)
   {
      mOffset = pos;
   }

private:
   const char *mBuffer;
   size_t mSize;
   size_t mOffset;
};
#pragma once
#include <cstdint>
#include "align.h"
#include "decaf_assert.h"

class FrameAllocator
{
public:
   FrameAllocator() :
      mBase(nullptr),
      mSize(0),
      mOffset(0)
   {
   }

   FrameAllocator(void *base, size_t size) :
      mBase(reinterpret_cast<uint8_t *>(base)),
      mSize(size),
      mOffset(0)
   {
   }

   bool
   empty() const
   {
      return mOffset == 0;
   }

   uint8_t *
   top() const
   {
      return mBase + mOffset;
   }

   void *
   allocate(size_t size,
            uint32_t alignment = 4)
   {
      // Ensure section alignment
      auto alignOffset = align_up(top(), alignment) - top();
      size += alignOffset;

      // Check we have enough size
      decaf_check(mOffset + size <= mSize);

      // Allocate data
      auto result = top() + alignOffset;
      mOffset += size;
      return result;
   }

   void
   reset()
   {
      mOffset = 0;
   }

protected:
   uint8_t *mBase;
   size_t mSize;
   size_t mOffset;
};

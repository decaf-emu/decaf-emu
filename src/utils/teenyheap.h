#pragma once
#include <assert.h>
#include <map>
#include <vector>
#include "utils/align.h"

class TeenyHeap
{
private:
   struct FreeBlock
   {
      uint8_t *start;
      uint8_t *end;
   };

public:
   TeenyHeap(void *buffer, size_t size) :
      mBuffer(static_cast<uint8_t*>(buffer)), mSize(size)
   {
      mFreeBlocks.emplace_back(FreeBlock { mBuffer, mBuffer + mSize });
   }

   void *
   alloc(size_t size, size_t alignment = 4)
   {
      // Pick first free block to allocate from
      auto itr = mFreeBlocks.begin();
      intptr_t alignOffset = 0;

      for (; itr != mFreeBlocks.end(); ++itr) {
         uint8_t *alignedStart = align_up(itr->start, alignment);

         if (static_cast<size_t>(itr->end - alignedStart) >= size) {
            // This block is big enough
            alignOffset = alignedStart - itr->start;
            break;
         }
      }

      if (itr == mFreeBlocks.end()) {
         // No big enough blocks
         return nullptr;
      }

      size_t alignedSize = size + alignOffset;
      uint8_t *basePtr = itr->start;
      uint8_t *alignedPtr = basePtr + alignOffset;

      // Remove or shrink the chosen free block
      if (itr->start - itr->end == alignedSize) {
         mFreeBlocks.erase(itr);
      } else {
         itr->start += alignedSize;
      }

      // Return the alignment bytes to the free list
      if (alignOffset > 0) {
         mFreeBlocks.emplace_back(FreeBlock { basePtr, basePtr + alignOffset });
      }

      // Save the size of this allocation
      mAllocSizes.emplace(alignedPtr, size);
      return alignedPtr;
   }

   void *
   realloc(void *ptr, size_t size, size_t alignment = 4)
   {
      auto ucptr = static_cast<uint8_t*>(ptr);
      auto itr = mAllocSizes.find(ucptr);
      assert(itr != mAllocSizes.end());

      // Only allow shrinking reallocs right now.
      assert(size <= itr->second);

      // Ensure alignments match
      assert(align_up(ucptr, alignment) == ucptr);

      // If sizes are equal, do nothing
      if (size == itr->second) {
         return ptr;
      }

      // Free the extra region
      releaseBlock({ ucptr + size, ucptr + itr->second });

      // Resize this allocation to be the right place
      itr->second = size;

      // Return the same pointer since we can only shrink right now
      return ptr;
   }

   void
   free(void *ptr)
   {
      auto ucptr = static_cast<uint8_t*>(ptr);
      auto itr = mAllocSizes.find(ucptr);
      assert(itr != mAllocSizes.end());

      releaseBlock({ ucptr, ucptr + itr->second });
      mAllocSizes.erase(itr);
   }

   std::pair<void*, void*>
   getRange() const
   {
      return std::make_pair((void*)mBuffer, (void*)(mBuffer + mSize));
   }

protected:
   void
   releaseBlock(FreeBlock block)
   {
      for (auto i = mFreeBlocks.begin(); i != mFreeBlocks.end(); ++i) {
         if (i->end == block.start) {
            block.start = i->start;
            mFreeBlocks.erase(i);
            return releaseBlock(block);
         }

         if (block.end == i->start) {
            block.end = i->end;
            mFreeBlocks.erase(i);
            return releaseBlock(block);
         }
      }

      mFreeBlocks.emplace_back(block);
   }

   uint8_t *mBuffer;
   size_t mSize;
   std::map<uint8_t*, size_t> mAllocSizes;
   std::vector<FreeBlock> mFreeBlocks;
};

#pragma once
#include "align.h"
#include "decaf_assert.h"
#include <algorithm>
#include <map>
#include <mutex>
#include <vector>

class TeenyHeap
{
private:
   struct MemoryBlock
   {
      uint8_t *start;
      size_t size;
   };

public:
   TeenyHeap() :
      mBuffer(nullptr),
      mSize(0)
   {
   }

   TeenyHeap(void *buffer, size_t size)
   {
      reset(buffer, size);
   }

   void
   reset(void *buffer, size_t size)
   {
      mAllocatedBlocks.clear();
      mFreeBlocks.clear();

      mBuffer = static_cast<uint8_t *>(buffer);
      mSize = size;
      mFreeBlocks.emplace_back(MemoryBlock { mBuffer, mSize });
   }

   size_t
   getLargestFreeSize()
   {
      auto largest = size_t { 0 };

      for (auto &block : mFreeBlocks) {
         largest = std::max(largest, block.size);
      }

      return largest;
   }

   size_t
   getTotalFreeSize()
   {
      auto total = size_t { 0 };

      for (auto &block : mFreeBlocks) {
         total += block.size;
      }

      return total;
   }

   void *
   alloc(size_t size, size_t alignment = 4)
   {
      std::unique_lock<std::mutex> lock(mMutex);
      auto adjSize = size;
      auto block = mFreeBlocks.begin();

      for (block = mFreeBlocks.begin(); block != mFreeBlocks.end(); ++block) {
         auto alignedDiff = align_up(block->start, alignment) - block->start;
         if (block->size - alignedDiff >= adjSize) {
            adjSize += alignedDiff;
            break;
         }
      }

      if (block == mFreeBlocks.end()) {
         return nullptr;
      }

      auto start = block->start;
      block->start += adjSize;
      block->size -= adjSize;

      // Erase block if it gets too small
      if (block->size <= sizeof(MemoryBlock) + 4) {
         adjSize += block->size;
         mFreeBlocks.erase(block);
      }

      // Allocate block
      auto alignedStart = align_up(start, alignment);
      mAllocatedBlocks.emplace(alignedStart, MemoryBlock { start, adjSize });

      return alignedStart;
   }

   void
   free(void *ptr)
   {
      std::unique_lock<std::mutex> lock(mMutex);
      auto ucptr = static_cast<uint8_t*>(ptr);
      auto itr = mAllocatedBlocks.find(ucptr);
      decaf_check(itr != mAllocatedBlocks.end());

      releaseBlock(itr->second);
      mAllocatedBlocks.erase(itr);
   }

protected:
   void
   releaseBlock(MemoryBlock block)
   {
      auto blockStart = block.start;
      auto blockEnd = block.start + block.size;

      for (auto &free : mFreeBlocks) {
         auto freeStart = free.start;
         auto freeEnd = free.start + free.size;

         if (blockStart == freeEnd) {
            free.size += block.size;
            return;
         }

         if (blockEnd == freeStart) {
            free.start = blockStart;
            free.size += block.size;
            return;
         }
      }

      mFreeBlocks.emplace_back(block);
   }

   uint8_t *mBuffer;
   size_t mSize;
   std::vector<MemoryBlock> mFreeBlocks;
   std::map<uint8_t *, MemoryBlock> mAllocatedBlocks;
   std::mutex mMutex;
};

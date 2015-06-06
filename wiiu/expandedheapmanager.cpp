#include <algorithm>
#include "expandedheapmanager.h"
#include "memory.h"

ExpandedHeapManager::ExpandedHeapManager(uint32_t address, uint32_t size, HeapFlags flags) :
   HeapManager(address, size, flags),
   mMode(HeapMode::FirstFree),
   mGroup(0)
{
   gMemory.alloc(address, size);
   addFreeBlock(0, size, HeapDirection::FromBottom);
}

ExpandedHeapManager::~ExpandedHeapManager()
{
   gMemory.free(mAddress);
}

uint32_t
ExpandedHeapManager::trimHeap()
{
   if (!mFreeBlocks.size()) {
      // No spare space
      return mSize;
   }

   auto lastFree = mFreeBlocks.back();

   if (lastFree.offset + lastFree.size != mSize) {
      // There is allocated data at top of heap, cannot shrink
      return mSize;
   } else {
      // Erase the last free block and reduce heap size
      mSize -= lastFree.size;
      mFreeBlocks.pop_back();

      // TODO: Free pages from system memory
      return mSize;
   }
}

uint32_t
ExpandedHeapManager::alloc(uint32_t size, int alignment)
{
   auto direction = HeapDirection::FromBottom;
   uint32_t offset = -1;

   if (alignment < 0) {
      alignment = -alignment;
      direction = HeapDirection::FromTop;
   }

   // Pad size to alignment
   size = (size + (alignment - 1)) & ~(alignment - 1);

   if (mMode == HeapMode::FirstFree) {
      offset = allocFirst(direction, size);
   } else if (mMode == HeapMode::NearestSize) {
      offset = allocNearestSize(direction, size);
   }

   if (offset == -1) {
      return 0;
   }

   // TODO: Adjust offset to alignment (this will break free)
   // offset = (offset + (alignment - 1)) & ~(alignment - 1);

   return offset + mAddress;
}

void
ExpandedHeapManager::free(uint32_t addr)
{
   auto block = findUsedBlockByAddress(addr);

   if (!block) {
      return;
   }

   auto nextFree = findFreeBlockByEndOffset(block->offset + block->size);
   auto prevFree = findFreeBlockByEndOffset(block->offset);

   if (nextFree && prevFree) {
      // Expand prevFree to consume nextFree
      prevFree->size += nextFree->size;
      eraseFreeBlockByOffset(nextFree->offset);
      nextFree = nullptr;
   } else if (nextFree) {
      // Expand nextFree backwards
      nextFree->offset -= block->size;
      nextFree->size += block->size;
   } else if (prevFree) {
      // Expand prevFree forwards
      prevFree->size += block->size;
   } else {
      // New free block
      addFreeBlock(block->offset, block->size, HeapDirection::FromBottom);
   }

   // Remove the used block
   eraseUsedBlockByOffset(block->offset);
}

uint32_t
ExpandedHeapManager::resizeBlock(uint32_t addr, uint32_t size)
{
   auto block = findUsedBlockByAddress(addr);

   if (!block) {
      return 0;
   }

   auto nextFree = findFreeBlockByOffset(block->offset + block->size);

   if (size < block->size) {
      auto dsize = block->size - size;

      if (nextFree) {
         // Expand next free block downwards
         nextFree->offset -= dsize;
         nextFree->size += dsize;
      } else {
         // Insert free block
         auto offset = block->offset + size;
         addFreeBlock(offset, dsize, HeapDirection::FromBottom);
      }

      block->size = size;
   } else if (size > block->size) {
      auto dsize = size - block->size;

      // We require a free block to grow in to
      if (!nextFree) {
         return 0;
      }

      if (dsize > nextFree->size) {
         // Not enough free space
         return 0;
      } else if (dsize == nextFree->size) {
         // Consume the whole next free block
         eraseFreeBlockByOffset(nextFree->offset);
         block->size = size;
      } else {
         // Consume start of next free block
         nextFree->offset += dsize;
         nextFree->size -= dsize;
         block->size = size;
      }
   }

   return block->size;
}

HeapMode
ExpandedHeapManager::getMode()
{
   return mMode;
}

uint8_t
ExpandedHeapManager::getGroup()
{
   return mGroup;
}

HeapMode
ExpandedHeapManager::setMode(HeapMode mode)
{
   auto old = mMode;
   mMode = mode;
   return old;
}

uint8_t
ExpandedHeapManager::setGroup(uint8_t group)
{
   auto old = mGroup;
   mGroup = group;
   return old;
}

uint32_t
ExpandedHeapManager::getBlockSize(uint32_t addr)
{
   auto block = findUsedBlockByAddress(addr);
   return block ? block->size : 0;
}

uint8_t
ExpandedHeapManager::getBlockGroup(uint32_t addr)
{
   auto block = findUsedBlockByAddress(addr);
   return block ? block->groupID : 0;
}

HeapDirection
ExpandedHeapManager::getBlockDirection(uint32_t addr)
{
   auto block = findUsedBlockByAddress(addr);
   return block ? static_cast<HeapDirection>(block->direction) : HeapDirection::FromBottom;
}

uint32_t
ExpandedHeapManager::getLargestFreeBlockSize(int alignment)
{
   uint32_t size = 0;
   
   // TODO: Use alignment
   for (auto &block : mFreeBlocks) {
      size = std::max(size, block.size);
   }

   return size;
}

uint32_t
ExpandedHeapManager::getTotalFreeSize()
{
   uint32_t size = 0;

   for (auto &block : mFreeBlocks) {
      size += block.size;
   }

   return size;
}

uint32_t
ExpandedHeapManager::allocNearestSize(HeapDirection direction, uint32_t size)
{
   uint32_t offset = -1;
   uint32_t nearestSize = size;

   if (direction == HeapDirection::FromBottom) {
      auto nearestBlock = mFreeBlocks.end();

      for (auto itr = mFreeBlocks.begin(); itr != mFreeBlocks.end(); ++itr) {
         auto &block = *itr;

         if (block.size < size) {
            continue;
         }

         auto dsize = block.size - size;

         if (dsize < nearestSize) {
            nearestSize = dsize;
            nearestBlock = itr;
         }

         break;
      }

      if (nearestBlock != mFreeBlocks.end()) {
         auto &block = *nearestBlock;

         // Insert used block before free block
         offset = block.offset;
         block.offset += size;
         block.size -= size;

         if (!block.size) {
            mFreeBlocks.erase(nearestBlock);
         }
      }
   } else if (direction == HeapDirection::FromTop) {
      auto nearestBlock = mFreeBlocks.rend();

      for (auto itr = mFreeBlocks.rbegin(); itr != mFreeBlocks.rend(); ++itr) {
         auto &block = *itr;

         if (block.size < size) {
            continue;
         }

         auto dsize = block.size - size;

         if (dsize < nearestSize) {
            nearestSize = dsize;
            nearestBlock = itr;
         }

         break;
      }

      if (nearestBlock != mFreeBlocks.rend()) {
         auto &block = *nearestBlock;

         // Insert used block after free block
         offset = block.offset + block.size - size;
         block.size -= size;

         if (!block.size) {
            mFreeBlocks.erase(std::next(nearestBlock).base());
         }
      }
   }

   // Add to used block list
   if (offset) {
      addUsedBlock(offset, size, direction);
   }

   return offset;
}

uint32_t
ExpandedHeapManager::allocFirst(HeapDirection direction, uint32_t size)
{
   uint32_t offset = -1;

   if (direction == HeapDirection::FromBottom) {
      for (auto itr = mFreeBlocks.begin(); itr != mFreeBlocks.end(); ++itr) {
         auto &block = *itr;

         if (block.size < size) {
            continue;
         }

         // Insert used block before free block
         offset = block.offset;
         block.offset += size;
         block.size -= size;

         if (!block.size) {
            mFreeBlocks.erase(itr);
         }

         break;
      }
   } else if (direction == HeapDirection::FromTop) {
      for (auto itr = mFreeBlocks.rbegin(); itr != mFreeBlocks.rend(); ++itr) {
         auto &block = *itr;

         if (block.size < size) {
            continue;
         }

         // Reduce size of the free block
         offset = block.offset + block.size - size;
         block.size -= size;

         if (!block.size) {
            mFreeBlocks.erase(std::next(itr).base());
         }

         break;
      }
   }

   // Add to used block list
   if (offset) {
      addUsedBlock(offset, size, direction);
   }

   return offset;
}

ExpHeapBlock *
ExpandedHeapManager::findUsedBlockByOffset(uint32_t offset)
{
   for (auto &block : mUsedBlocks) {
      if (block.offset == offset) {
         return &block;
      }
   }

   return nullptr;
}

ExpHeapBlock *
ExpandedHeapManager::findUsedBlockByAddress(uint32_t address)
{
   return findUsedBlockByOffset(address - mAddress);
}

ExpHeapBlock *
ExpandedHeapManager::findFreeBlockByEndOffset(uint32_t offset)
{
   for (auto &block : mFreeBlocks) {
      if (block.offset + block.size == offset) {
         return &block;
      }
   }

   return nullptr;
}

ExpHeapBlock *
ExpandedHeapManager::findFreeBlockByOffset(uint32_t offset)
{
   for (auto &block : mFreeBlocks) {
      if (block.offset == offset) {
         return &block;
      }
   }

   return nullptr;
}

ExpHeapBlock *
ExpandedHeapManager::findFreeBlockByAddress(uint32_t address)
{
   return findFreeBlockByOffset(address - mAddress);
}

void ExpandedHeapManager::eraseFreeBlockByOffset(uint32_t offset)
{
   for (auto itr = mFreeBlocks.begin(); itr != mFreeBlocks.end(); ++itr) {
      if (itr->offset == offset) {
         mFreeBlocks.erase(itr);
         return;
      }
   }
}

void
ExpandedHeapManager::eraseUsedBlockByOffset(uint32_t offset)
{
   for (auto itr = mUsedBlocks.begin(); itr != mUsedBlocks.end(); ++itr) {
      if (itr->offset == offset) {
         mFreeBlocks.erase(itr);
         return;
      }
   }
}

void
ExpandedHeapManager::addFreeBlock(uint32_t offset, uint32_t size, HeapDirection direction)
{
   ExpHeapBlock block = { offset, size, static_cast<uint16_t>(direction), mGroup };
   mFreeBlocks.insert(std::upper_bound(mFreeBlocks.begin(), mFreeBlocks.end(), block), block);
}

void
ExpandedHeapManager::addUsedBlock(uint32_t offset, uint32_t size, HeapDirection direction)
{
   ExpHeapBlock block = { offset, size, static_cast<uint16_t>(direction), mGroup };
   mUsedBlocks.insert(std::upper_bound(mUsedBlocks.begin(), mUsedBlocks.end(), block), block);
}

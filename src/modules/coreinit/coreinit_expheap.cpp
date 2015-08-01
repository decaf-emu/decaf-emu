#include "coreinit.h"
#include "coreinit_expheap.h"
#include "system.h"

#pragma pack(push, 1)

struct ExpandedHeapBlock
{
   uint32_t addr;
   uint32_t size;
   uint16_t group;
   HeapDirection direction;
   p32<ExpandedHeapBlock> next;
   p32<ExpandedHeapBlock> prev;
};

struct ExpandedHeap : CommonHeap
{
   uint32_t size;
   uint32_t bottom;
   uint32_t top;
   uint16_t group;
   HeapMode mode;
   p32<ExpandedHeapBlock> freeBlockList;
   p32<ExpandedHeapBlock> usedBlockList;
};

#pragma pack(pop)

static const uint32_t
minimumBlockSize = sizeof(ExpandedHeapBlock) + 4;

static p32<ExpandedHeapBlock>
findBlock(p32<ExpandedHeapBlock> head, uint32_t addr)
{
   for (auto block = head; block; block = block->next) {
      if (block->addr == addr) {
         return block;
      }
   }

   return nullptr;
}

static void
eraseBlock(p32<ExpandedHeapBlock> &head, p32<ExpandedHeapBlock> block)
{
   if (block == head) {
      head = block->next;
   } else {
      if (block->prev) {
         block->prev->next = block->next;
      }

      if (block->next) {
         block->next->prev = block->prev;
      }
   }
}

static void
insertBlock(p32<ExpandedHeapBlock> &head, p32<ExpandedHeapBlock> block)
{
   p32<ExpandedHeapBlock> insertAfter = nullptr;

   for (auto itr = head; itr; itr = itr->next) {
      if (itr->addr < block->addr) {
         insertAfter = itr;
      } else {
         break;
      }
   }

   if (!insertAfter) {
      block->next = head;
      block->prev = nullptr;

      if (head) {
         head->prev = block;
      }

      head = block;
   } else {
      if (insertAfter->next) {
         insertAfter->next->prev = block;
      }

      block->next = insertAfter->next;
      block->prev = insertAfter;
      insertAfter->next = block;
   }
}

static void
replaceBlock(p32<ExpandedHeapBlock> &head, p32<ExpandedHeapBlock> old, p32<ExpandedHeapBlock> block)
{
   if (!old) {
      insertBlock(head, block);
   } else {
      if (head == old) {
         head = block;
      } else {
         old->prev->next = block;
      }

      if (old->next) {
         old->next->prev = block;
      }

      block->next = old->next;
      block->prev = old->prev;
   }
}

static p32<ExpandedHeapBlock>
getTail(p32<ExpandedHeapBlock> block)
{
   p32<ExpandedHeapBlock> tail;

   for (; block; block = block->next) {
      tail = block;
   }

   return tail;
}

ExpandedHeap *
MEMCreateExpHeap(ExpandedHeap *heap, uint32_t size)
{
   return MEMCreateExpHeapEx(heap, size, 0);
}

ExpandedHeap *
MEMCreateExpHeapEx(ExpandedHeap *heap, uint32_t size, uint16_t flags)
{
   // Allocate memory
   auto base = gMemory.untranslate(heap);
   gMemory.alloc(base, size);

   // Setup state
   heap->size = size;
   heap->bottom = base;
   heap->top = base + size;
   heap->mode = HeapMode::FirstFree;
   heap->group = 0;
   heap->usedBlockList = nullptr;
   heap->freeBlockList = make_p32<ExpandedHeapBlock>(base + sizeof(ExpandedHeap));
   heap->freeBlockList->addr = static_cast<uint32_t>(heap->freeBlockList);
   heap->freeBlockList->size = heap->size - sizeof(ExpandedHeap);
   heap->freeBlockList->next = nullptr;
   heap->freeBlockList->prev = nullptr;

   // Setup common header
   MEMiInitHeapHead(heap, HeapType::ExpandedHeap, heap->freeBlockList->addr, heap->freeBlockList->addr + heap->freeBlockList->size);
   return heap;
}

ExpandedHeap *
MEMDestroyExpHeap(ExpandedHeap *heap)
{
   MEMiFinaliseHeap(heap);
   gMemory.free(gMemory.untranslate(heap));
   return heap;
}

void
MEMiDumpExpHeap(ExpandedHeap *heap)
{
   gLog->debug("MEMiDumpExpHeap({:8x})", gMemory.untranslate(heap));
   gLog->debug("Status Address Size Group");

   for (auto block = heap->freeBlockList; block; block = block->next) {
      gLog->debug("FREE {:8x} {:8x} {:d}", block->addr, block->size, block->group);
   }

   for (auto block = heap->usedBlockList; block; block = block->next) {
      gLog->debug("USED {:8x} {:8x} {:d}", block->addr, block->size, block->group);
   }
}

void *
MEMAllocFromExpHeap(ExpandedHeap *heap, uint32_t size)
{
   ScopedSpinLock lock(&heap->lock);
   return MEMAllocFromExpHeapEx(heap, size, 4);
}

void *
MEMAllocFromExpHeapEx(ExpandedHeap *heap, uint32_t size, int alignment)
{
   ScopedSpinLock lock(&heap->lock);
   p32<ExpandedHeapBlock> freeBlock = nullptr, usedBlock = nullptr;
   auto direction = HeapDirection::FromBottom;
   uint32_t base;

   if (alignment < 0) {
      alignment = -alignment;
      direction = HeapDirection::FromTop;
   }

   // Add size for block header and alignment
   size += sizeof(ExpandedHeapBlock);
   size += alignment;

   if (heap->mode == HeapMode::FirstFree) {
      if (direction == HeapDirection::FromBottom) {
         // Find first block large enough from bottom of heap
         for (auto block = heap->freeBlockList; block; block = block->next) {
            if (block->size < size) {
               continue;
            }

            freeBlock = block;
            break;
         }
      } else if (direction == HeapDirection::FromTop) {
         // Find first block large enough from top of heap
         for (auto block = getTail(heap->freeBlockList); block; block = block->prev) {
            if (block->size < size) {
               continue;
            }

            freeBlock = block;
            break;
         }
      }
   } else if (heap->mode == HeapMode::NearestSize) {
      uint32_t nearestSize = -1;

      if (direction == HeapDirection::FromBottom) {
         // Find block nearest in size from bottom of heap
         for (auto block = heap->freeBlockList; block; block = block->next) {
            if (block->size < size) {
               continue;
            }

            if (block->size - size < nearestSize) {
               nearestSize = block->size - size;
               freeBlock = block;
            }
         }
      } else if (direction == HeapDirection::FromTop) {
         // Find block nearest in size from top of heap
         for (auto block = getTail(heap->freeBlockList); block; block = block->prev) {
            if (block->size < size) {
               continue;
            }

            if (block->size - size < nearestSize) {
               nearestSize = block->size - size;
               freeBlock = block;
            }
         }
      }
   }

   if (!freeBlock) {
      gLog->error("MEMAllocFromExpHeapEx failed, no free block found");
      MEMiDumpExpHeap(heap);
      return 0;
   }

   if (direction == HeapDirection::FromBottom) {
      // Reduce freeblock size
      base = freeBlock->addr;
      freeBlock->size -= size;

      if (freeBlock->size < minimumBlockSize) {
         // Absorb free block as it is too small
         size += freeBlock->size;
         eraseBlock(heap->freeBlockList, freeBlock);
      } else {
         auto freeSize = freeBlock->size;

         // Replace free block
         auto old = freeBlock;
         freeBlock = make_p32<ExpandedHeapBlock>(base + size);
         freeBlock->addr = base + size;
         freeBlock->size = freeSize;
         replaceBlock(heap->freeBlockList, old, freeBlock);
      }
   } else if (direction == HeapDirection::FromTop) {
      // Reduce freeblock size
      freeBlock->size -= size;
      base = freeBlock->addr + freeBlock->size;

      if (freeBlock->size < minimumBlockSize) {
         // Absorb free block as it is too small
         size += freeBlock->size;
         eraseBlock(heap->freeBlockList, freeBlock);
      }
   }

   // Create a new used block
   auto aligned = alignUp(base + static_cast<uint32_t>(sizeof(ExpandedHeapBlock)), alignment);
   usedBlock = make_p32<ExpandedHeapBlock>(aligned - static_cast<uint32_t>(sizeof(ExpandedHeapBlock)));
   usedBlock->addr = base;
   usedBlock->size = size;
   usedBlock->group = heap->group;
   usedBlock->direction = direction;
   insertBlock(heap->usedBlockList, usedBlock);
   return make_p32<void>(aligned);
}

void
MEMFreeToExpHeap(ExpandedHeap *heap, void *address)
{
   ScopedSpinLock lock(&heap->lock);
   auto base = gMemory.untranslate(address);

   if (!base) {
      return;
   }

   // Get the block header
   base = base - static_cast<uint32_t>(sizeof(ExpandedHeapBlock));

   // Remove used blocked
   auto usedBlock = make_p32<ExpandedHeapBlock>(base);
   auto addr = usedBlock->addr;
   auto size = usedBlock->size;
   eraseBlock(heap->usedBlockList, usedBlock);

   // Create free block
   auto freeBlock = make_p32<ExpandedHeapBlock>(addr);
   freeBlock->addr = addr;
   freeBlock->size = size;
   insertBlock(heap->freeBlockList, freeBlock);

   // Merge with next free if contiguous
   auto nextFree = freeBlock->next;

   if (nextFree && nextFree->addr == freeBlock->addr + freeBlock->size) {
      freeBlock->size += nextFree->size;
      eraseBlock(heap->freeBlockList, nextFree);
   }

   // Merge with previous free if contiguous
   auto prevFree = freeBlock->prev;

   if (prevFree && freeBlock->addr == prevFree->addr + prevFree->size) {
      prevFree->size += freeBlock->size;
      eraseBlock(heap->freeBlockList, freeBlock);
   }
}

HeapMode
MEMSetAllocModeForExpHeap(ExpandedHeap *heap, HeapMode mode)
{
   ScopedSpinLock lock(&heap->lock);
   auto previous = heap->mode;
   heap->mode = mode;
   return previous;
}

HeapMode
MEMGetAllocModeForExpHeap(ExpandedHeap *heap)
{
   ScopedSpinLock lock(&heap->lock);
   return heap->mode;
}

uint32_t
MEMAdjustExpHeap(ExpandedHeap *heap)
{
   ScopedSpinLock lock(&heap->lock);

   // Find the last free block
   auto lastFree = getTail(heap->freeBlockList);

   if (!lastFree) {
      return heap->size;
   }

   // Erase the last free block
   heap->size -= lastFree->size;
   eraseBlock(heap->freeBlockList, lastFree);

   return heap->size;
}

uint32_t
MEMResizeForMBlockExpHeap(ExpandedHeap *heap, p32<void> mblock, uint32_t size)
{
   ScopedSpinLock lock(&heap->lock);

   // Get the block header
   auto address = static_cast<uint32_t>(mblock);
   auto base = address - static_cast<uint32_t>(sizeof(ExpandedHeapBlock));

   auto block = make_p32<ExpandedHeapBlock>(base);
   auto nextAddr = block->addr + block->size;

   auto freeBlock = findBlock(heap->freeBlockList, nextAddr);
   auto freeBlockSize = 0u;

   auto dataSize = (block->addr + block->size) - address;
   auto difSize = static_cast<int32_t>(size) - static_cast<int32_t>(dataSize);
   auto newSize = block->size + difSize;

   if (difSize == 0) {
      // No difference, return current size
      return size;
   } else if (difSize > 0) {
      if (!freeBlock) {
         // No free block to expand into, return fail
         return 0;
      } else if (freeBlock->size < static_cast<uint32_t>(difSize)) {
         // Free block is too small, return fail
         return 0;
      } else {
         if (freeBlock->size - difSize < minimumBlockSize) {
            // The free block will be smaller than minimum size, so just absorb it completely
            freeBlockSize = 0;
            newSize = freeBlock->size;
         } else {
            // Free block is large enough, we just reduce its size
            freeBlockSize = freeBlock->size - difSize;
         }
      }
   } else if (difSize < 0) {
      if (freeBlock) {
         // Increase size of free block
         freeBlockSize = freeBlock->size - difSize;
      } else if (difSize < minimumBlockSize) {
         // We can't fit a new free block in the gap, so return current size
         return block->size;
      } else {
         // Create a new free block in the gap
         freeBlockSize = -difSize;
      }
   }

   // Update free block
   if (freeBlockSize) {
      auto old = freeBlock;
      freeBlock = make_p32<ExpandedHeapBlock>(block->addr + newSize);
      freeBlock->addr = block->addr + newSize;
      freeBlock->size = freeBlockSize;
      replaceBlock(heap->freeBlockList, old, freeBlock);
   } else {
      // We have totally consumed the free block
      eraseBlock(heap->freeBlockList, freeBlock);
   }

   // Resize block
   block->size = newSize;
   return size;
}

uint32_t
MEMGetTotalFreeSizeForExpHeap(ExpandedHeap *heap)
{
   ScopedSpinLock lock(&heap->lock);
   auto size = 0u;

   for (auto block = heap->freeBlockList; block; block = block->next) {
      size += block->size;
   }

   return size;
}

uint32_t
MEMGetAllocatableSizeForExpHeap(ExpandedHeap *heap)
{
   ScopedSpinLock lock(&heap->lock);
   return MEMGetAllocatableSizeForExpHeapEx(heap, 4);
}

uint32_t
MEMGetAllocatableSizeForExpHeapEx(ExpandedHeap *heap, int alignment)
{
   ScopedSpinLock lock(&heap->lock);
   auto size = 0u;

   // Find largest block
   for (auto block = heap->freeBlockList; block; block = block->next) {
      if (block->size > size) {
         size = block->size;
      }
   }

   // Ensure it is big enough for alignment
   if (size < sizeof(ExpandedHeapBlock) + alignment) {
      return 0;
   }

   // Return the allocatable size
   size -= sizeof(ExpandedHeapBlock);
   size -= alignment;

   // TODO: Figure out why WUP-P-ARKE crashes if we have more than 994mb ram
   if (size > 0x3e200000) {
      return 0x3e200000;
   } else {
      return size;
   }
}

uint16_t
MEMSetGroupIDForExpHeap(ExpandedHeap *heap, uint16_t id)
{
   ScopedSpinLock lock(&heap->lock);
   auto previous = heap->group;
   heap->group = id;
   return previous;
}

uint16_t
MEMGetGroupIDForExpHeap(ExpandedHeap *heap)
{
   ScopedSpinLock lock(&heap->lock);
   return heap->group;
}

uint32_t
MEMGetSizeForMBlockExpHeap(p32<void> addr)
{
   auto block = make_p32<ExpandedHeapBlock>(static_cast<uint32_t>(addr) - sizeof(ExpandedHeapBlock));
   return block->size;
}

uint16_t
MEMGetGroupIDForMBlockExpHeap(p32<void> addr)
{
   auto block = make_p32<ExpandedHeapBlock>(static_cast<uint32_t>(addr) - sizeof(ExpandedHeapBlock));
   return block->group;
}

HeapDirection
MEMGetAllocDirForMBlockExpHeap(p32<void> addr)
{
   auto block = make_p32<ExpandedHeapBlock>(static_cast<uint32_t>(addr) - sizeof(ExpandedHeapBlock));
   return block->direction;
}

void
CoreInit::registerExpHeapFunctions()
{
   RegisterKernelFunction(MEMCreateExpHeap);
   RegisterKernelFunction(MEMCreateExpHeapEx);
   RegisterKernelFunction(MEMDestroyExpHeap);
   RegisterKernelFunction(MEMAllocFromExpHeap);
   RegisterKernelFunction(MEMAllocFromExpHeapEx);
   RegisterKernelFunction(MEMFreeToExpHeap);
   RegisterKernelFunction(MEMSetAllocModeForExpHeap);
   RegisterKernelFunction(MEMGetAllocModeForExpHeap);
   RegisterKernelFunction(MEMAdjustExpHeap);
   RegisterKernelFunction(MEMResizeForMBlockExpHeap);
   RegisterKernelFunction(MEMGetTotalFreeSizeForExpHeap);
   RegisterKernelFunction(MEMGetAllocatableSizeForExpHeap);
   RegisterKernelFunction(MEMGetAllocatableSizeForExpHeapEx);
   RegisterKernelFunction(MEMSetGroupIDForExpHeap);
   RegisterKernelFunction(MEMGetGroupIDForExpHeap);
   RegisterKernelFunction(MEMGetSizeForMBlockExpHeap);
   RegisterKernelFunction(MEMGetGroupIDForMBlockExpHeap);
   RegisterKernelFunction(MEMGetAllocDirForMBlockExpHeap);
}

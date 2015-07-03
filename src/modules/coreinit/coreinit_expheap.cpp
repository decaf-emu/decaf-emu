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
eraseBlock(p32<ExpandedHeapBlock> &head, p32<ExpandedHeapBlock> freeBlock)
{
   if (freeBlock == head) {
      head = freeBlock->next;
   } else {
      freeBlock->prev->next = freeBlock->next;
   }
}

static void
insertBlock(p32<ExpandedHeapBlock> &head, p32<ExpandedHeapBlock> usedBlock)
{
   p32<ExpandedHeapBlock> insertAfter = nullptr;

   for (auto block = head; block; block = block->next) {
      if (block->addr < usedBlock->addr) {
         insertAfter = block;
      } else {
         break;
      }
   }

   if (!insertAfter) {
      usedBlock->next = head;

      if (head) {
         head->prev = usedBlock;
      }

      head = usedBlock;
   } else {
      usedBlock->next = insertAfter->next;
      usedBlock->prev = insertAfter;
      insertAfter->next = usedBlock;
   }
}

static void
replaceBlock(p32<ExpandedHeapBlock> &head, p32<ExpandedHeapBlock> old, p32<ExpandedHeapBlock> freeBlock)
{
   assert(freeBlock->size < 0xff000000);
   if (!old) {
      insertBlock(head, freeBlock);
   } else {
      if (head == old) {
         head = freeBlock;
      } else {
         old->prev->next = freeBlock;
      }

      if (old->next) {
         old->next->prev = freeBlock;
      }

      freeBlock->next = old->next;
      freeBlock->prev = old->prev;
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
   heap->freeBlockList->addr = base + sizeof(ExpandedHeap);
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
   xLog() << "Status Address  Size     Group";

   for (auto block = heap->freeBlockList; block; block = block->next) {
      xLog() << "FREE   " << Log::hex(block->addr) << " " << Log::hex(block->size) << " " << block->group;
   }

   for (auto block = heap->usedBlockList; block; block = block->next) {
      xLog() << "USED   " << Log::hex(block->addr) << " " << Log::hex(block->size) << " " << block->group;
   }
}

void *
MEMAllocFromExpHeap(ExpandedHeap *heap, uint32_t size)
{
   return MEMAllocFromExpHeapEx(heap, size, 4);
}

void *
MEMAllocFromExpHeapEx(ExpandedHeap *heap, uint32_t size, int alignment)
{
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
      xError() << "MEMAllocFromExpHeapEx failed, no free block found";
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

         // Erase old block
         eraseBlock(heap->freeBlockList, freeBlock);

         // Create new free block
         freeBlock = make_p32<ExpandedHeapBlock>(base + size);
         freeBlock->addr = base + size;
         freeBlock->size = freeSize;
         insertBlock(heap->freeBlockList, freeBlock);
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
   auto base = gMemory.untranslate(address);

   if (!base) {
      return;
   }

   // Get the block header
   base = base - static_cast<uint32_t>(sizeof(ExpandedHeapBlock));
   auto block = make_p32<ExpandedHeapBlock>(base);

   // Remove from used list
   eraseBlock(heap->usedBlockList, block);

   // Insert to free list
   insertBlock(heap->freeBlockList, block);

   // Merge with next free if contiguous
   auto nextFree = block->next;

   if (nextFree && nextFree->addr == block->addr + block->size) {
      block->size += nextFree->size;
      eraseBlock(heap->freeBlockList, nextFree);
   }

   // Merge with previous free if contiguous
   auto prevFree = block->prev;

   if (prevFree && block->addr == prevFree->addr + prevFree->size) {
      prevFree->size += block->size;
      eraseBlock(heap->freeBlockList, block);
   }
}

HeapMode
MEMSetAllocModeForExpHeap(ExpandedHeap *heap, HeapMode mode)
{
   auto previous = heap->mode;
   heap->mode = mode;
   return previous;
}

HeapMode
MEMGetAllocModeForExpHeap(ExpandedHeap *heap)
{
   return heap->mode;
}

uint32_t
MEMAdjustExpHeap(ExpandedHeap *heap)
{
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
MEMResizeForMBlockExpHeap(ExpandedHeap *heap, p32<void> address, uint32_t size)
{
   // Get the block header
   auto base = static_cast<uint32_t>(address) - static_cast<uint32_t>(sizeof(ExpandedHeapBlock));
   auto block = make_p32<ExpandedHeapBlock>(base);

   auto nextAddr = block->addr + block->size;
   auto freeBlock = findBlock(heap->freeBlockList, nextAddr);
   auto freeBlockSize = 0u;
   auto difSize = static_cast<int32_t>(size) - static_cast<int32_t>(block->size);

   if (difSize == 0) {
      // No difference, return current size
      return size;
   } else if (difSize > 0) {
      if (!freeBlock) {
         // No free block to expand into, return fail
         return 0;
      } else if (freeBlock->size - difSize) {
         // Free block is too small, return fail
         return 0;
      } else {
         if (freeBlock->size - difSize < minimumBlockSize) {
            // The free block will be smaller than minimum size, so just absorb it completely
            freeBlockSize = 0;
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
      freeBlock = make_p32<ExpandedHeapBlock>(base + size);
      freeBlock->addr = base + size;
      freeBlock->size = freeBlockSize;
      replaceBlock(heap->freeBlockList, old, freeBlock);
   } else {
      // We have totally consumed the free block
      eraseBlock(heap->freeBlockList, freeBlock);
   }

   // Resize block
   block->size = size;
   return size;
}

uint32_t
MEMGetTotalFreeSizeForExpHeap(ExpandedHeap *heap)
{
   auto size = 0u;

   for (auto block = heap->freeBlockList; block; block = block->next) {
      size += block->size;
   }

   return size;
}

uint32_t
MEMGetAllocatableSizeForExpHeap(ExpandedHeap *heap)
{
   return MEMGetAllocatableSizeForExpHeapEx(heap, 4);
}

uint32_t
MEMGetAllocatableSizeForExpHeapEx(ExpandedHeap *heap, int alignment)
{
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
   return size;
}

uint16_t
MEMSetGroupIDForExpHeap(ExpandedHeap *heap, uint16_t id)
{
   auto previous = heap->group;
   heap->group = id;
   return previous;
}

uint16_t
MEMGetGroupIDForExpHeap(ExpandedHeap *heap)
{
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

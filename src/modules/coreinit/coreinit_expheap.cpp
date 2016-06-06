#include "coreinit.h"
#include "coreinit_expheap.h"
#include "cpu/mem.h"
#include "system.h"
#include "common/align.h"
#include "common/virtual_ptr.h"

namespace coreinit
{

#pragma pack(push, 1)

struct ExpandedHeapBlock
{
   uint32_t addr;
   uint32_t size;
   uint16_t group;
   MEMExpHeapDirection direction;
   virtual_ptr<ExpandedHeapBlock> next;
   virtual_ptr<ExpandedHeapBlock> prev;
};

struct ExpandedHeap : CommonHeap
{
   uint32_t size;
   uint32_t bottom;
   uint32_t top;
   uint16_t group;
   MEMExpHeapMode mode;
   virtual_ptr<ExpandedHeapBlock> freeBlockList;
   virtual_ptr<ExpandedHeapBlock> usedBlockList;
};

#pragma pack(pop)

static const uint32_t
sMinimumBlockSize = sizeof(ExpandedHeapBlock) + 4;

static virtual_ptr<ExpandedHeapBlock>
findBlock(virtual_ptr<ExpandedHeapBlock> head, uint32_t addr)
{
   for (auto block = head; block; block = block->next) {
      if (block->addr == addr) {
         return block;
      }
   }

   return nullptr;
}

static void
eraseBlock(virtual_ptr<ExpandedHeapBlock> &head, virtual_ptr<ExpandedHeapBlock> block)
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
insertBlock(virtual_ptr<ExpandedHeapBlock> &head, virtual_ptr<ExpandedHeapBlock> block)
{
   virtual_ptr<ExpandedHeapBlock> insertAfter = nullptr;

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
replaceBlock(virtual_ptr<ExpandedHeapBlock> &head, virtual_ptr<ExpandedHeapBlock> old, virtual_ptr<ExpandedHeapBlock> block)
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

static virtual_ptr<ExpandedHeapBlock>
getTail(virtual_ptr<ExpandedHeapBlock> block)
{
   virtual_ptr<ExpandedHeapBlock> tail;

   for (; block; block = block->next) {
      tail = block;
   }

   return tail;
}


/**
 * Initialise an expanded heap.
 */
ExpandedHeap *
MEMCreateExpHeap(ExpandedHeap *heap, uint32_t size)
{
   return MEMCreateExpHeapEx(heap, size, 0);
}


/**
 * Initialise an expanded heap.
 *
 * Adds it to the list of active heaps.
 */
ExpandedHeap *
MEMCreateExpHeapEx(ExpandedHeap *heap, uint32_t size, uint16_t flags)
{
   // Setup state
   auto base = mem::untranslate(heap);
   heap->size = size;
   heap->bottom = base;
   heap->top = base + size;
   heap->mode = MEMExpHeapMode::FirstFree;
   heap->group = 0;
   heap->usedBlockList = nullptr;
   heap->freeBlockList = make_virtual_ptr<ExpandedHeapBlock>(base + sizeof(ExpandedHeap));
   heap->freeBlockList->addr = heap->freeBlockList.getAddress();
   heap->freeBlockList->size = heap->size - sizeof(ExpandedHeap);
   heap->freeBlockList->next = nullptr;
   heap->freeBlockList->prev = nullptr;

   // Setup common header
   MEMiInitHeapHead(heap, MEMiHeapTag::ExpandedHeap, heap->freeBlockList->addr, heap->freeBlockList->addr + heap->freeBlockList->size);
   return heap;
}


/**
 * Destroy expanded heap.
 *
 * Remove it from the list of active heaps.
 */
ExpandedHeap *
MEMDestroyExpHeap(ExpandedHeap *heap)
{
   MEMiFinaliseHeap(heap);
   return heap;
}


/**
 * Print debug information about the expanded heap.
 */
void
MEMiDumpExpHeap(ExpandedHeap *heap)
{
   gLog->debug("MEMiDumpExpHeap({:8x})", mem::untranslate(heap));
   gLog->debug("Status Address Size Group");

   for (auto block = heap->freeBlockList; block; block = block->next) {
      gLog->debug("FREE {:8x} {:8x} {:d}", block->addr, block->size, block->group);
   }

   for (auto block = heap->usedBlockList; block; block = block->next) {
      gLog->debug("USED {:8x} {:8x} {:d}", block->addr, block->size, block->group);
   }
}


/**
 * Allocate memory from an expanded heap with default alignment
 */
void *
MEMAllocFromExpHeap(ExpandedHeap *heap, uint32_t size)
{
   ScopedSpinLock lock(&heap->lock);
   return MEMAllocFromExpHeapEx(heap, size, 4);
}


/**
 * Allocate aligned memory from an expanded heap
 *
 * Sets the memory block group ID to the current active group ID.
 * If alignment is negative the memory is allocated from the top of the heap.
 * If alignment is positive the memory is allocated from the bottom of the heap.
 */
void *
MEMAllocFromExpHeapEx(ExpandedHeap *heap, uint32_t size, int alignment)
{
   ScopedSpinLock lock(&heap->lock);
   virtual_ptr<ExpandedHeapBlock> freeBlock, usedBlock;
   auto direction = MEMExpHeapDirection::FromBottom;
   uint32_t base;

   if (alignment < 0) {
      alignment = -alignment;
      direction = MEMExpHeapDirection::FromTop;
   }

   // Add size for block header and alignment
   uint32_t originalSize = size;
   size += sizeof(ExpandedHeapBlock);
   size += alignment;

   if (heap->mode == MEMExpHeapMode::FirstFree) {
      if (direction == MEMExpHeapDirection::FromBottom) {
         // Find first block large enough from bottom of heap
         for (auto block = heap->freeBlockList; block; block = block->next) {
            if (block->size < size) {
               continue;
            }

            freeBlock = block;
            break;
         }
      } else {  // direction == MEMExpHeapDirection::FromTop
         // Find first block large enough from top of heap
         for (auto block = getTail(heap->freeBlockList); block; block = block->prev) {
            if (block->size < size) {
               continue;
            }

            freeBlock = block;
            break;
         }
      }
   } else if (heap->mode == MEMExpHeapMode::NearestSize) {
      uint32_t nearestSize = -1;

      if (direction == MEMExpHeapDirection::FromBottom) {
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
      } else {  // direction == MEMExpHeapDirection::FromTop
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
      gLog->error("MEMAllocFromExpHeapEx failed, no free block found for size {:08x} ({:08x}+{:x}+{:x})", size, originalSize, sizeof(ExpandedHeapBlock), alignment);
      MEMiDumpExpHeap(heap);
      return nullptr;
   }

   if (direction == MEMExpHeapDirection::FromBottom) {
      // Reduce freeblock size
      base = freeBlock->addr;
      freeBlock->size -= size;

      if (freeBlock->size < sMinimumBlockSize) {
         // Absorb free block as it is too small
         size += freeBlock->size;
         eraseBlock(heap->freeBlockList, freeBlock);
      } else {
         auto freeSize = freeBlock->size;

         // Replace free block
         auto old = freeBlock;
         freeBlock = make_virtual_ptr<ExpandedHeapBlock>(base + size);
         freeBlock->addr = base + size;
         freeBlock->size = freeSize;
         replaceBlock(heap->freeBlockList, old, freeBlock);
      }
   } else {  // direction == MEMExpHeapDirection::FromTop
      // Reduce freeblock size
      freeBlock->size -= size;
      base = freeBlock->addr + freeBlock->size;

      if (freeBlock->size < sMinimumBlockSize) {
         // Absorb free block as it is too small
         size += freeBlock->size;
         eraseBlock(heap->freeBlockList, freeBlock);
      }
   }

   // Create a new used block
   auto aligned = align_up(base + static_cast<uint32_t>(sizeof(ExpandedHeapBlock)), alignment);
   usedBlock = make_virtual_ptr<ExpandedHeapBlock>(aligned - static_cast<uint32_t>(sizeof(ExpandedHeapBlock)));
   usedBlock->addr = base;
   usedBlock->size = size;
   usedBlock->group = heap->group;
   usedBlock->direction = direction;
   insertBlock(heap->usedBlockList, usedBlock);
   return make_virtual_ptr<void>(aligned);
}


/**
 * Free a memory block in an expanded heap
 */
void
MEMFreeToExpHeap(ExpandedHeap *heap, void *block)
{
   ScopedSpinLock lock(&heap->lock);
   auto base = mem::untranslate(block);

   if (!base) {
      return;
   }

   if (base < heap->bottom || base >= heap->top) {
      gLog->warn("FreeToExpHeap outside heap region; {:08x} not within {:08x}-{:08x}", base, heap->bottom, heap->top);
      return;
   }

   // Get the block header
   base = base - static_cast<uint32_t>(sizeof(ExpandedHeapBlock));

   // Remove used blocked
   auto usedBlock = make_virtual_ptr<ExpandedHeapBlock>(base);
   auto addr = usedBlock->addr;
   auto size = usedBlock->size;
   eraseBlock(heap->usedBlockList, usedBlock);

   // Create free block
   auto freeBlock = make_virtual_ptr<ExpandedHeapBlock>(addr);
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


/**
 * Set the allocate mode for the expanded heap
 *
 * The two valid modes are
 * - FirstFree: allocate the first free block which is large enough for the requested size.
 * - NearestSize: allocate the free block which is nearest in size to the requested size.
 */
MEMExpHeapMode
MEMSetAllocModeForExpHeap(ExpandedHeap *heap, MEMExpHeapMode mode)
{
   ScopedSpinLock lock(&heap->lock);
   auto previous = heap->mode;
   heap->mode = mode;
   return previous;
}


/**
 * Returns the allocate mode for an expanded heap.
 */
MEMExpHeapMode
MEMGetAllocModeForExpHeap(ExpandedHeap *heap)
{
   ScopedSpinLock lock(&heap->lock);
   return heap->mode;
}


/**
 * Trim free blocks from the end of the expanded heap.
 *
 * Reduces the size of the heap memory region to the end of the last allocated block.
 * Returns the new size of the heap.
 */
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


/**
 * Resize an allocated memory block.
 */
uint32_t
MEMResizeForMBlockExpHeap(ExpandedHeap *heap, uint8_t *mblock, uint32_t size)
{
   ScopedSpinLock lock(&heap->lock);

   // Get the block header
   auto address = mem::untranslate(mblock);
   auto base = address - static_cast<uint32_t>(sizeof(ExpandedHeapBlock));

   auto block = make_virtual_ptr<ExpandedHeapBlock>(base);
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
         if (freeBlock->size - difSize < sMinimumBlockSize) {
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
      } else if (difSize < sMinimumBlockSize) {
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
      freeBlock = make_virtual_ptr<ExpandedHeapBlock>(block->addr + newSize);
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


/**
 * Return the total free size of an expanded heap.
 */
uint32_t
MEMGetTotalFreeSizeForExpHeap(ExpandedHeap *heap)
{
   ScopedSpinLock lock(&heap->lock);
   auto size = 0u;

   for (auto block = heap->freeBlockList; block; block = block->next) {
      size += block->size - sizeof(ExpandedHeapBlock);
   }

   return size;
}


/**
 * Return the largest allocatable memory block in an expanded heap.
 */
uint32_t
MEMGetAllocatableSizeForExpHeap(ExpandedHeap *heap)
{
   ScopedSpinLock lock(&heap->lock);
   return MEMGetAllocatableSizeForExpHeapEx(heap, 4);
}


/**
 * Return the largest allocatable aligned memory block in an expanded heap.
 */
uint32_t
MEMGetAllocatableSizeForExpHeapEx(ExpandedHeap *heap, int alignment)
{
   ScopedSpinLock lock(&heap->lock);
   auto size = 0u;

   // Find largest block
   for (auto block = heap->freeBlockList; block; block = block->next) {
      size = std::max<uint32_t>(block->size, size);
   }

   // Ensure it is big enough for alignment
   if (size < sizeof(ExpandedHeapBlock) + alignment) {
      return 0;
   }

   // Return the allocatable size
   size -= sizeof(ExpandedHeapBlock);
   size -= alignment;

   // HACK: WUP-P-ARKE behaves badly when it has more than 994 MB of memory...
   size = std::min(size, 994u * 1024 * 1024);

   return size;
}


/**
 * Set the current group ID for an expanded heap.
 *
 * This group ID will be used for all newly allocated memory blocks.
 */
uint16_t
MEMSetGroupIDForExpHeap(ExpandedHeap *heap, uint16_t id)
{
   ScopedSpinLock lock(&heap->lock);
   auto previous = heap->group;
   heap->group = id;
   return previous;
}


/**
 * Get the current group id for an expanded heap.
 */
uint16_t
MEMGetGroupIDForExpHeap(ExpandedHeap *heap)
{
   ScopedSpinLock lock(&heap->lock);
   return heap->group;
}


/**
 * Return the size of an allocated memory block from an expanded heap.
 */
uint32_t
MEMGetSizeForMBlockExpHeap(uint8_t *addr)
{
   auto block = reinterpret_cast<ExpandedHeapBlock *>(addr - sizeof(ExpandedHeapBlock));
   return block->size;
}


/**
 * Return the group id of an allocated memory block from an expanded heap.
 */
uint16_t
MEMGetGroupIDForMBlockExpHeap(uint8_t *addr)
{
   auto block = reinterpret_cast<ExpandedHeapBlock *>(addr - sizeof(ExpandedHeapBlock));
   return block->group;
}


/**
 * Return the allocation direction of an allocated memory block from an expanded heap.
 */
MEMExpHeapDirection
MEMGetAllocDirForMBlockExpHeap(uint8_t *addr)
{
   auto block = reinterpret_cast<ExpandedHeapBlock *>(addr - sizeof(ExpandedHeapBlock));
   return block->direction;
}


void
Module::registerExpHeapFunctions()
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
   RegisterKernelFunction(MEMiDumpExpHeap);
}

} // namespace coreinit

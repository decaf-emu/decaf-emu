#include "coreinit.h"
#include "coreinit_memheap.h"
#include "coreinit_memblockheap.h"
#include "coreinit_spinlock.h"
#include "common/align.h"
#include "common/decaf_assert.h"
#include "common/structsize.h"
#include <array>

#pragma optimize("", off)

namespace coreinit
{

/**
 * Initialise block heap.
 */
MEMBlockHeap *
MEMInitBlockHeap(MEMBlockHeap *heap,
                 void *start,
                 void *end,
                 MEMBlockHeapTracking *blocks,
                 uint32_t size,
                 uint32_t flags)
{
   if (!heap || !start || !end || start >= end) {
      return nullptr;
   }

   auto dataStart = reinterpret_cast<uint8_t *>(start);
   auto dataEnd = reinterpret_cast<uint8_t *>(end);

   // Register heap
   internal::registerHeap(&heap->header,
                          MEMHeapTag::BlockHeap,
                          dataStart,
                          dataEnd,
                          flags);

   // Setup default tracker
   heap->defaultTrack.blockCount = 1;
   heap->defaultTrack.blocks = &heap->defaultBlock;

   // Setup default block
   heap->defaultBlock.start = dataStart;
   heap->defaultBlock.end = dataEnd;
   heap->defaultBlock.isFree = TRUE;
   heap->defaultBlock.next = nullptr;
   heap->defaultBlock.prev = nullptr;

   // Add default block to block list
   heap->firstBlock = &heap->defaultBlock;
   heap->lastBlock = &heap->defaultBlock;

   MEMAddBlockHeapTracking(heap, blocks, size);
   return heap;
}


/**
 * Destroy block heap.
 */
void *
MEMDestroyBlockHeap(MEMBlockHeap *heap)
{
   if (!heap || heap->header.tag != MEMHeapTag::BlockHeap) {
      return nullptr;
   }

   std::memset(heap, 0, sizeof(MEMBlockHeap));
   internal::unregisterHeap(&heap->header);
   return heap;
}


/**
 * Adds more tracking block memory to block heap.
 */
int
MEMAddBlockHeapTracking(MEMBlockHeap *heap,
                        MEMBlockHeapTracking *tracking,
                        uint32_t size)
{
   if (!heap || !tracking || heap->header.tag != MEMHeapTag::BlockHeap) {
      return -4;
   }

   // Size must be enough to contain at least 1 tracking structure and 1 block
   if (size < sizeof(MEMBlockHeapTracking) + sizeof(MEMBlockHeapBlock)) {
      return -4;
   }

   auto blockCount = (size - sizeof(MEMBlockHeapTracking)) / sizeof(MEMBlockHeapBlock);
   auto blocks = reinterpret_cast<MEMBlockHeapBlock *>(tracking + 1);

   // Setup tracking data
   tracking->blockCount = blockCount;
   tracking->blocks = blocks;

   // Setup block linked list
   for (auto i = 0u; i < blockCount; ++i) {
      auto &block = blocks[i];
      block.prev = nullptr;
      block.next = &blocks[i + 1];
   }

   // Insert at start of block list
   internal::HeapLock lock(&heap->header);
   blocks[blockCount - 1].next = heap->firstFreeBlock;
   heap->firstFreeBlock = blocks;
   heap->numFreeBlocks += blockCount;

   return 0;
}

static MEMBlockHeapBlock *
findBlockOwning(MEMBlockHeap *heap,
                void *data)
{
   auto addr = reinterpret_cast<uint8_t *>(data);

   if (addr < heap->header.dataStart) {
      return nullptr;
   }

   if (addr >= heap->header.dataEnd) {
      return nullptr;
   }

   auto distFromEnd = heap->header.dataEnd.get() - addr;
   auto distFromStart = addr - heap->header.dataStart;

   if (distFromStart < distFromEnd) {
      // Look forward from firstBlock
      auto block = heap->firstBlock;

      while (block) {
         if (block->end > addr) {
            return block;
         }

         block = block->next;
      }
   } else {
      // Go backwards from lastBlock
      auto block = heap->lastBlock;

      while (block) {
         if (block->start <= addr) {
            return block;
         }

         block = block->prev;
      }
   }

   return nullptr;
}

static bool
allocInsideBlock(MEMBlockHeap *heap,
                 MEMBlockHeapBlock *block,
                 uint8_t *start,
                 uint32_t size)
{
   // Ensure we are actually inside this block
   auto end = start + size;

   if (size == 0 || end > block->end) {
      return false;
   }

   // First lets check we have enough free blocks
   auto needFreeBlocks = 0u;

   if (start != block->start) {
      needFreeBlocks++;
   }

   if (end != block->end) {
      needFreeBlocks++;
   }

   if (heap->numFreeBlocks < needFreeBlocks) {
      return false;
   }

   // Create free block for remaining memory at start of block
   if (start != block->start) {
      // Get a new free block
      auto freeBlock = heap->firstFreeBlock;
      heap->firstFreeBlock = freeBlock->next;
      heap->numFreeBlocks--;

      // Setup free block
      freeBlock->start = block->start;
      freeBlock->end = start;
      freeBlock->isFree = TRUE;
      freeBlock->prev = block->prev;
      freeBlock->next = block;

      if (freeBlock->prev) {
         freeBlock->prev->next = freeBlock;
      } else {
         heap->firstBlock = freeBlock;
      }

      // Adjust current block
      block->start = start;
      block->prev = freeBlock;
   }

   // Create free block for remaining memory at end of block
   if (end != block->end) {
      // Get a new free block
      auto freeBlock = heap->firstFreeBlock;
      heap->firstFreeBlock = freeBlock->next;
      heap->numFreeBlocks--;

      // Setup free block
      freeBlock->start = end;
      freeBlock->end = block->end;
      freeBlock->isFree = TRUE;
      freeBlock->prev = block;
      freeBlock->next = block->next;

      if (block->next) {
         block->next->prev = freeBlock;
      } else {
         heap->lastBlock = freeBlock;
      }

      // Adjust current block
      block->end = end;
      block->next = freeBlock;
   }

   // Set intial memory values
   auto heapAttribs = heap->header.attribs.value();

   if (heapAttribs.zeroAllocated()) {
      std::memset(block->start, 0, size);
   } else if (heapAttribs.debugMode()) {
      auto value = MEMGetFillValForHeap(MEMHeapFillType::Allocated);
      std::memset(block->start, value, size);
   }

   // Set block to allocated and return success!
   block->isFree = FALSE;
   return true;
}


/**
 * Try to allocate from block heap at a specific address.
 */
void *
MEMAllocFromBlockHeapAt(MEMBlockHeap *heap,
                        void *addr,
                        uint32_t size)
{
   if (!heap || !addr || !size || heap->header.tag != MEMHeapTag::BlockHeap) {
      return nullptr;
   }

   if (!heap->firstFreeBlock) {
      return nullptr;
   }

   internal::HeapLock lock(&heap->header);

   auto block = findBlockOwning(heap, addr);

   if (!block) {
      gLog->warn("MEMAllocFromBlockHeapAt: Could not find block containing addr 0x{:08X}", mem::untranslate(addr));
      return nullptr;
   }

   if (!block->isFree) {
      gLog->warn("MEMAllocFromBlockHeapAt: Requested address is not free 0x{:08X}", mem::untranslate(addr));
      return nullptr;
   }

   if (!allocInsideBlock(heap, block, reinterpret_cast<uint8_t *>(addr), size)) {
      return nullptr;
   }

   return addr;
}


/**
 * Allocate from block heap.
 */
void *
MEMAllocFromBlockHeapEx(MEMBlockHeap *heap,
                        uint32_t size,
                        int32_t align)
{
   if (!heap || !size || heap->header.tag != MEMHeapTag::BlockHeap) {
      return nullptr;
   }

   internal::HeapLock lock(&heap->header);
   MEMBlockHeapBlock *block = nullptr;
   uint8_t *alignedStart = nullptr;
   void *result = nullptr;

   if (align == 0) {
      align = 4;
   }

   if (align >= 0) {
      // Find first free block with enough size
      for (block = heap->firstBlock; block; block = block->next) {
         if (block->isFree) {
            alignedStart = align_up(block->start.get(), align);

            if (alignedStart + size < block->end) {
               break;
            }
         }
      }
   } else {
      // Find last free block with enough size
      for (block = heap->lastBlock; block; block = block->prev) {
         if (block->isFree) {
            auto alignedStart = align_down(block->end.get() - size, -align);

            if (alignedStart >= block->start) {
               break;
            }
         }
      }
   }

   if (allocInsideBlock(heap, block, alignedStart, size)) {
      result = alignedStart;
   }

   return result;
}


/**
 * Free memory back to block heap.
 */
void
MEMFreeToBlockHeap(MEMBlockHeap *heap,
                   void *data)
{
   if (!heap || !data || heap->header.tag != MEMHeapTag::BlockHeap) {
      return;
   }

   internal::HeapLock lock(&heap->header);
   auto block = findBlockOwning(heap, data);

   if (!block) {
      gLog->warn("MEMFreeToBlockHeap: Could not find block containing data 0x{:08X}", mem::untranslate(data));
      return;
   }

   if (block->isFree) {
      gLog->warn("MEMFreeToBlockHeap: Tried to free an already free block");
      return;
   }

   if (block->start != data) {
      gLog->warn("MEMFreeToBlockHeap: Tried to free block 0x{:08X} from middle 0x{:08X}", mem::untranslate(block->start), mem::untranslate(data));
      return;
   }

   if (heap->header.attribs.value().debugMode()) {
      auto fill = MEMGetFillValForHeap(MEMHeapFillType::Freed);
      auto size = block->end.get() - block->start.get();
      std::memset(block->start, fill, size);
   }

   // Merge with previous free block if possible
   if (auto prev = block->prev) {
      if (prev->isFree) {
         prev->end = block->end;
         prev->next = block->next;

         if (auto next = prev->next) {
            next->prev = prev;
         } else {
            heap->lastBlock = prev;
         }

         block->prev = nullptr;
         block->next = heap->firstFreeBlock;
         heap->numFreeBlocks++;
         heap->firstFreeBlock = block;

         block = prev;
      }
   }

   block->isFree = TRUE;

   // Merge with next free block if possible
   if (auto next = block->next) {
      if (next->isFree) {
         block->end = next->end;
         block->next = next->next;

         if (next->next) {
            next->next->prev = block;
         } else {
            heap->lastBlock = block;
         }

         next->next = heap->firstFreeBlock;
         heap->firstFreeBlock = next;
         heap->numFreeBlocks++;
      }
   }
}


/**
 * Find the largest possible allocatable size in block heap for an alignment.
 */
uint32_t
MEMGetAllocatableSizeForBlockHeapEx(MEMBlockHeap *heap,
                                    int32_t align)
{
   if (!heap || heap->header.tag != MEMHeapTag::BlockHeap) {
      return 0;
   }

   internal::HeapLock lock(&heap->header);
   auto allocatableSize = 0u;

   // Adjust align
   if (align < 0) {
      align = -align;
   } else if (align == 0) {
      align = 4;
   }

   for (auto block = heap->firstBlock; block; block = block->next) {
      if (!block->isFree) {
         continue;
      }

      // Align start address and check it is still inside block
      auto startAddr = block->start.getAddress();
      auto endAddr = block->end.getAddress();
      auto alignedStart = align_up(startAddr, align);

      if (alignedStart >= endAddr) {
         continue;
      }

      // See if this block is largest free block so far
      auto freeSize = endAddr - alignedStart;

      if (freeSize > allocatableSize) {
         allocatableSize = freeSize;
      }
   }

   return allocatableSize;
}


/**
 * Return number of tracking blocks remaining in heap.
 */
uint32_t
MEMGetTrackingLeftInBlockHeap(MEMBlockHeap *heap)
{
   if (!heap || heap->header.tag != MEMHeapTag::BlockHeap) {
      return 0;
   }

   return heap->numFreeBlocks;
}


/**
 * Return total free size in the heap.
 */
uint32_t
MEMGetTotalFreeSizeForBlockHeap(MEMBlockHeap *heap)
{
   if (!heap || heap->header.tag != MEMHeapTag::BlockHeap) {
      return 0;
   }

   internal::HeapLock lock(&heap->header);
   auto freeSize = 0u;

   for (auto block = heap->firstBlock; block; block = block->next) {
      if (!block->isFree) {
         continue;
      }

      auto startAddr = block->start.getAddress();
      auto endAddr = block->end.getAddress();
      freeSize += endAddr - startAddr;
   }

   return freeSize;
}

void
Module::registerBlockHeapFunctions()
{
   RegisterKernelFunction(MEMInitBlockHeap);
   RegisterKernelFunction(MEMDestroyBlockHeap);
   RegisterKernelFunction(MEMAddBlockHeapTracking);
   RegisterKernelFunction(MEMAllocFromBlockHeapAt);
   RegisterKernelFunction(MEMAllocFromBlockHeapEx);
   RegisterKernelFunction(MEMFreeToBlockHeap);
   RegisterKernelFunction(MEMGetAllocatableSizeForBlockHeapEx);
   RegisterKernelFunction(MEMGetTrackingLeftInBlockHeap);
   RegisterKernelFunction(MEMGetTotalFreeSizeForBlockHeap);
}

} // coreinit

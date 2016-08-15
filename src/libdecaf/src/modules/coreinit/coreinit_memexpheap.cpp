#include "coreinit.h"
#include "coreinit_memexpheap.h"
#include "common/bitfield.h"
#include "libcpu/mem.h"
#include "common/align.h"
#include "virtual_ptr.h"

namespace coreinit
{

static const auto
FreeTag = 0x4654; // 'FR'

static const auto
UsedTag = 0x5544; // 'UD'

static uint8_t *
getBlockMemStart(MEMExpHeapBlock *block)
{
   auto attribs = static_cast<MEMExpHeapBlockAttribs>(block->attribs);
   return reinterpret_cast<uint8_t*>(block) - attribs.alignment();
}

static uint8_t *
getBlockMemEnd(MEMExpHeapBlock *block)
{
   return reinterpret_cast<uint8_t*>(block) + sizeof(MEMExpHeapBlock) + block->blockSize;
}

static uint8_t *
getBlockDataStart(MEMExpHeapBlock *block)
{
   return reinterpret_cast<uint8_t*>(block) + sizeof(MEMExpHeapBlock);
}

static MEMExpHeapBlock *
getUsedMemBlock(void * mem)
{
   auto block = reinterpret_cast<MEMExpHeapBlock *>(mem) - 1;
   decaf_check(block->tag == UsedTag);
   return block;
}

static bool
listContainsBlock(MEMExpHeapBlockList *list,
                  MEMExpHeapBlock *block)
{
   for (auto i = list->head; i; i = i->next) {
      if (i == block) {
         return true;
      }
   }

   return false;
}

static void
insertBlock(MEMExpHeapBlockList *list,
            MEMExpHeapBlock *prev,
            MEMExpHeapBlock *block)
{
   decaf_check(!block->prev);
   decaf_check(!block->next);

   if (!prev) {
      block->next = list->head;
      block->prev = nullptr;

      list->head = block;
   } else {
      block->next = prev->next;
      block->prev = prev;

      prev->next = block;
   }

   if (block->next) {
      block->next->prev = block;
   } else {
      list->tail = block;
   }
}

static void
removeBlock(MEMExpHeapBlockList *list,
            MEMExpHeapBlock *block)
{
   decaf_check(listContainsBlock(list, block));

   if (block->prev) {
      block->prev->next = block->next;
   } else {
      list->head = block->next;
   }

   if (block->next) {
      block->next->prev = block->prev;
   } else {
      list->tail = block->prev;
   }

   block->prev = nullptr;
   block->next = nullptr;
}

static uint32_t
getAlignedBlockSize(MEMExpHeapBlock *block,
                    uint32_t alignment,
                    MEMExpHeapDirection dir)
{
   if (dir == MEMExpHeapDirection::FromStart) {
      auto dataStart = reinterpret_cast<uint8_t *>(block) + sizeof(MEMExpHeapBlock);
      auto dataEnd = dataStart + block->blockSize;
      auto alignedDataStart = align_up(dataStart, alignment);

      if (alignedDataStart >= dataEnd) {
         return 0;
      }

      return static_cast<uint32_t>(dataEnd - alignedDataStart);
   } else if (dir == MEMExpHeapDirection::FromEnd) {
      auto dataStart = reinterpret_cast<uint8_t *>(block) + sizeof(MEMExpHeapBlock);
      auto dataEnd = dataStart + block->blockSize;
      auto alignedDataEnd = align_down(dataEnd, alignment);

      if (alignedDataEnd <= dataStart) {
         return 0;
      }

      return static_cast<uint32_t>(alignedDataEnd - dataStart);
   } else {
      decaf_abort("Unexpected ExpHeap direction");
   }
}

static MEMExpHeapBlock *
createUsedBlockFromFreeBlock(MEMExpHeap *heap,
                             MEMExpHeapBlock *freeBlock,
                             uint32_t size,
                             uint32_t alignment,
                             MEMExpHeapDirection dir)
{
   auto heapAttribs = heap->header.attribs.value();
   auto expHeapAttribs = heap->attribs.value();
   auto freeBlockAttribs = freeBlock->attribs.value();

   auto freeBlockPrev = freeBlock->prev;
   auto freeMemStart = getBlockMemStart(freeBlock);
   auto freeMemEnd = getBlockMemEnd(freeBlock);

   // Free blocks should never have alignment...
   decaf_check(!freeBlockAttribs.alignment());
   removeBlock(&heap->freeList, freeBlock);

   // Find where we are going to start
   uint8_t *alignedDataStart = nullptr;

   if (dir == MEMExpHeapDirection::FromStart) {
      alignedDataStart = align_up(freeMemStart + sizeof(MEMExpHeapBlock), alignment);
   } else if (dir == MEMExpHeapDirection::FromEnd) {
      alignedDataStart = align_down(freeMemEnd - size, alignment);
   } else {
      decaf_abort("Unexpected ExpHeap direction");
   }

   // Grab the block header pointer and validate everything is sane
   auto alignedBlock = reinterpret_cast<MEMExpHeapBlock *>(alignedDataStart) - 1;
   decaf_check(alignedDataStart - sizeof(MEMExpHeapBlock) >= freeMemStart);
   decaf_check(alignedDataStart + size <= freeMemEnd);

   // Calculate the alignment waste
   auto topSpaceRemain = (alignedDataStart - freeMemStart) - sizeof(MEMExpHeapBlock);
   auto bottomSpaceRemain = (freeMemEnd - alignedDataStart) - size;

   if (expHeapAttribs.reuseAlignSpace() || dir == MEMExpHeapDirection::FromEnd) {
      // If the user wants to reuse the alignment space, or we allocated from the bottom,
      //  we should try to release the top space back to the heap free list.
      if (topSpaceRemain > sizeof(MEMExpHeapBlock) + 4) {
         // We have enough room to put some of the memory back to the free list
         freeBlock = reinterpret_cast<MEMExpHeapBlock *>(freeMemStart);
         freeBlock->attribs = MEMExpHeapBlockAttribs::get(0);
         freeBlock->blockSize = topSpaceRemain - sizeof(MEMExpHeapBlock);
         freeBlock->next = nullptr;
         freeBlock->prev = nullptr;
         freeBlock->tag = FreeTag;

         insertBlock(&heap->freeList, freeBlockPrev, freeBlock);
         topSpaceRemain = 0;
      }
   }

   if (expHeapAttribs.reuseAlignSpace() || dir == MEMExpHeapDirection::FromStart) {
      // If the user wants to reuse the alignment space, or we allocated from the top,
      //  we should try to release the bottom space back to the heap free list.
      if (bottomSpaceRemain > sizeof(MEMExpHeapBlock) + 4) {
         // We have enough room to put some of the memory back to the free list
         freeBlock = reinterpret_cast<MEMExpHeapBlock *>(freeMemEnd - bottomSpaceRemain);
         freeBlock->attribs = MEMExpHeapBlockAttribs::get(0);
         freeBlock->blockSize = bottomSpaceRemain - sizeof(MEMExpHeapBlock);
         freeBlock->next = nullptr;
         freeBlock->prev = nullptr;
         freeBlock->tag = FreeTag;

         insertBlock(&heap->freeList, freeBlockPrev, freeBlock);
         bottomSpaceRemain = 0;
      }
   }

   // Update the structure with the new allocation
   alignedBlock->attribs = MEMExpHeapBlockAttribs::get(0)
      .alignment(static_cast<uint32_t>(topSpaceRemain))
      .allocDir(dir);
   alignedBlock->blockSize = size + bottomSpaceRemain;
   alignedBlock->prev = nullptr;
   alignedBlock->next = nullptr;
   alignedBlock->tag = UsedTag;

   insertBlock(&heap->usedList, nullptr, alignedBlock);

   if (heapAttribs.zeroAllocated()) {
      memset(alignedDataStart, 0, size);
   } else if (heapAttribs.debugMode()) {
      auto fillVal = MEMGetFillValForHeap(MEMHeapFillType::Allocated);
      memset(alignedDataStart, fillVal, size);
   }

   return alignedBlock;
}

void
releaseMemory(MEMExpHeap *heap,
              uint8_t *memStart,
              uint8_t *memEnd)
{
   decaf_check(memEnd - memStart >= sizeof(MEMExpHeapBlock) + 4);
   auto heapAttribs = heap->header.attribs.value();

   // Fill the released memory with debug data if needed
   if (heapAttribs.debugMode()) {
      auto fillVal = MEMGetFillValForHeap(MEMHeapFillType::Freed);
      memset(memStart, fillVal, memEnd - memStart);
   }

   // Find the preceeding block to the memory we are releasing
   MEMExpHeapBlock *prevBlock = nullptr;
   MEMExpHeapBlock *nextBlock = heap->freeList.head;

   for (auto block = heap->freeList.head; block; block = block->next) {
      if (getBlockMemStart(block) < memStart) {
         prevBlock = block;
         nextBlock = block->next;
      } else if (block >= prevBlock) {
         break;
      }
   }

   MEMExpHeapBlock *freeBlock = nullptr;

   if (prevBlock) {
      // If there is a previous block, we need to check if we
      //  should just steal that block rather than making one.
      auto prevMemEnd = getBlockMemEnd(prevBlock);

      if (memStart == prevMemEnd) {
         // Previous block absorbs the new memory
         prevBlock->blockSize += memEnd - memStart;

         // Our free block becomes the previous one
         freeBlock = prevBlock;
      }
   }

   if (!freeBlock) {
      // We did not steal the previous block to free into,
      //  we need to allocate our own here.
      freeBlock = reinterpret_cast<MEMExpHeapBlock *>(memStart);
      freeBlock->attribs = MEMExpHeapBlockAttribs::get(0);
      freeBlock->blockSize = (memEnd - memStart) - sizeof(MEMExpHeapBlock);
      freeBlock->next = nullptr;
      freeBlock->prev = nullptr;
      freeBlock->tag = FreeTag;

      insertBlock(&heap->freeList, prevBlock, freeBlock);
   }

   if (nextBlock) {
      // If there is a next block, we need to possibly merge it down
      //  into this one.
      auto nextBlockStart = getBlockMemStart(nextBlock);

      if (nextBlockStart == memEnd) {
         // The next block needs to be merged into the freeBlock, as they
         //  are directly adjacent to each other in memory.
         auto nextBlockEnd = getBlockMemEnd(nextBlock);
         freeBlock->blockSize += nextBlockEnd - nextBlockStart;

         removeBlock(&heap->freeList, nextBlock);
      }
   }
}

MEMExpHeap *
MEMCreateExpHeapEx(void *base,
                   uint32_t size,
                   uint32_t flags)
{
   decaf_check(base);

   auto heapData = static_cast<uint8_t*>(base);
   auto alignedStart = align_up(heapData, 4);
   auto alignedEnd = align_down(heapData + size, 4);

   if (alignedEnd < alignedStart || alignedEnd - alignedStart < 0x6C) {
      // Not enough room for the header
      return nullptr;
   }

   // Get our heap header
   auto heap = reinterpret_cast<MEMExpHeap*>(alignedStart);

   // Register Heap
   internal::registerHeap(&heap->header,
                          MEMHeapTag::ExpandedHeap,
                          alignedStart + sizeof(MEMExpHeap),
                          alignedEnd,
                          flags);

   // Create an initial block of the data
   auto dataStart = alignedStart + sizeof(MEMExpHeap);
   auto firstBlock = reinterpret_cast<MEMExpHeapBlock*>(dataStart);

   firstBlock->attribs = MEMExpHeapBlockAttribs::get(0);
   firstBlock->blockSize = (alignedEnd - dataStart) - sizeof(MEMExpHeapBlock);
   firstBlock->next = nullptr;
   firstBlock->prev = nullptr;
   firstBlock->tag = FreeTag;

   heap->freeList.head = firstBlock;
   heap->freeList.tail = firstBlock;
   heap->usedList.head = nullptr;
   heap->usedList.tail = nullptr;

   heap->groupId = 0;
   heap->attribs = MEMExpHeapAttribs::get(0);

   return heap;
}

void *
MEMDestroyExpHeap(MEMExpHeap *heap)
{
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::ExpandedHeap);
   internal::unregisterHeap(&heap->header);
   return heap;
}

void *
MEMAllocFromExpHeapEx(MEMExpHeap *heap,
                      uint32_t size,
                      int32_t alignment)
{
   decaf_check(heap->header.tag == MEMHeapTag::ExpandedHeap);
   auto expHeapFlags = heap->attribs.value();

   if (size == 0) {
      size = 1;
   }

   decaf_check(alignment != 0);

   internal::HeapLock lock(&heap->header);
   MEMExpHeapBlock *newBlock = nullptr;

   size = align_up(size, 4);

   if (alignment > 0) {
      MEMExpHeapBlock *foundBlock = nullptr;
      uint32_t bestAlignedSize = 0xFFFFFFFF;

      alignment = std::max(4, alignment);
      decaf_check((alignment & 0x3) == 0);

      for (auto block = heap->freeList.head; block; block = block->next) {
         auto alignedSize = getAlignedBlockSize(block, alignment, MEMExpHeapDirection::FromStart);

         if (alignedSize >= size) {
            if (expHeapFlags.allocMode() == MEMExpHeapMode::FirstFree) {
               foundBlock = block;
               break;
            } else {
               if (alignedSize < bestAlignedSize) {
                  foundBlock = block;
                  bestAlignedSize = alignedSize;
               }
            }
         }
      }

      if (foundBlock) {
         newBlock = createUsedBlockFromFreeBlock(heap, foundBlock, size, alignment, MEMExpHeapDirection::FromStart);
      }
   } else {
      alignment = std::max(4, -alignment);
      decaf_check((alignment & 0x3) == 0);

      MEMExpHeapBlock *foundBlock = nullptr;
      auto bestAlignedSize = 0xFFFFFFFFu;

      for (auto block = heap->freeList.head; block; block = block->next) {
         auto alignedSize = getAlignedBlockSize(block, alignment, MEMExpHeapDirection::FromEnd);

         if (alignedSize >= size) {
            if (expHeapFlags.allocMode() == MEMExpHeapMode::FirstFree) {
               foundBlock = block;
               break;
            } else {
               if (alignedSize < bestAlignedSize) {
                  foundBlock = block;
                  bestAlignedSize = alignedSize;
               }
            }
         }
      }

      if (foundBlock) {
         newBlock = createUsedBlockFromFreeBlock(heap, foundBlock, size, alignment, MEMExpHeapDirection::FromEnd);
      }
   }

   if (!newBlock) {
      MEMDumpHeap(&heap->header);
      return nullptr;
   }

   return getBlockDataStart(newBlock);
}

void
MEMFreeToExpHeap(MEMExpHeap *heap,
                 void *mem)
{
   decaf_check(heap->header.tag == MEMHeapTag::ExpandedHeap);

   if (!mem) {
      return;
   }

   internal::HeapLock lock(&heap->header);

   // Find the block
   auto dataStart = reinterpret_cast<uint8_t *>(mem);
   auto block = reinterpret_cast<MEMExpHeapBlock*>(dataStart - sizeof(MEMExpHeapBlock));

   // Get the bounding region for this block
   auto memStart = getBlockMemStart(block);
   auto memEnd = getBlockMemEnd(block);

   // Remove the block from the used list
   removeBlock(&heap->usedList, block);

   // Release the memory back to the heap free list
   releaseMemory(heap, memStart, memEnd);
}

MEMExpHeapMode
MEMSetAllocModeForExpHeap(MEMExpHeap *heap,
                          MEMExpHeapMode mode)
{
   internal::HeapLock lock(&heap->header);

   auto expHeapAttribs = heap->attribs.value();
   heap->attribs = expHeapAttribs.allocMode(mode);
   return expHeapAttribs.allocMode();
}

MEMExpHeapMode
MEMGetAllocModeForExpHeap(MEMExpHeap *heap)
{
   internal::HeapLock lock(&heap->header);

   auto expHeapAttribs = heap->attribs.value();
   return expHeapAttribs.allocMode();
}

uint32_t
MEMAdjustExpHeap(MEMExpHeap *heap)
{
   internal::HeapLock lock(&heap->header);
   auto lastFreeBlock = heap->freeList.tail;

   if (!lastFreeBlock) {
      return 0;
   }

   auto blockData = reinterpret_cast<uint8_t*>(lastFreeBlock.get()) + sizeof(MEMExpHeapBlock);

   if (blockData + lastFreeBlock->blockSize != heap->header.dataEnd) {
      // This block is not for the end of the heap
      return 0;
   }

   // Remove the block from the free list
   decaf_check(!lastFreeBlock->next);

   if (lastFreeBlock->prev) {
      lastFreeBlock->prev->next = nullptr;
   }

   // Move the heaps end pointer to the true start point of this block
   heap->header.dataEnd = getBlockMemStart(lastFreeBlock);

   auto heapMemStart = reinterpret_cast<uint8_t*>(heap);
   auto heapMemEnd = reinterpret_cast<uint8_t*>(heap->header.dataEnd.get());
   return static_cast<uint32_t>(heapMemEnd - heapMemStart);
}

uint32_t
MEMResizeForMBlockExpHeap(MEMExpHeap *heap,
                          uint8_t *address,
                          uint32_t size)
{
   internal::HeapLock lock(&heap->header);
   size = align_up(size, 4);

   auto heapAttribs = heap->header.attribs.value();
   auto block = getUsedMemBlock(address);

   if (size < block->blockSize) {
      auto releasedSpace = block->blockSize - size;

      if (releasedSpace > sizeof(MEMExpHeapBlock) + 0x4) {
         auto releasedMemEnd = getBlockMemEnd(block);
         auto releasedMemStart = releasedMemEnd - releasedSpace;

         block->blockSize -= releasedSpace;

         releaseMemory(heap, releasedMemStart, releasedMemEnd);
      }
   } else if (size > block->blockSize) {
      auto blockMemEnd = getBlockMemEnd(block);

      MEMExpHeapBlock *freeBlock = nullptr;
      for (auto i = heap->freeList.head; i; i = i->next) {
         auto freeBlockMemStart = getBlockMemStart(i);

         if (freeBlockMemStart == blockMemEnd) {
            freeBlock = i;
            break;
         }

         // Free list is sorted, so we only need to search a little bit
         if (freeBlockMemStart > blockMemEnd) {
            break;
         }
      }

      if (!freeBlock) {
         return 0;
      }

      // Grab the data we need from the free block
      auto freeBlockMemStart = getBlockMemStart(freeBlock);
      auto freeBlockMemEnd = getBlockMemEnd(freeBlock);
      auto freeMemSize = freeBlockMemEnd - freeBlockMemStart;

      // Drop the free block from the list of free regions
      removeBlock(&heap->freeList, freeBlock);

      // Adjust the sizing of the free area and the block
      auto newAllocSize = (size - block->blockSize);
      freeMemSize -= newAllocSize;
      block->blockSize = size;

      if (heapAttribs.zeroAllocated()) {
         memset(freeBlockMemStart, 0, newAllocSize);
      } else if (heapAttribs.debugMode()) {
         auto fillVal = MEMGetFillValForHeap(MEMHeapFillType::Allocated);
         memset(freeBlockMemStart, fillVal, newAllocSize);
      }

      // If we have enough room to create a new free block, lets release
      //  the memory back to the heap.  Otherwise we just tack the remainder
      //  onto the end of the block we resized.
      if (freeMemSize >= sizeof(MEMExpHeapBlock) + 0x4) {
         releaseMemory(heap, freeBlockMemEnd - freeMemSize, freeBlockMemEnd);
      } else {
         block->blockSize += freeMemSize;
      }
   }

   return block->blockSize;
}

uint32_t
MEMGetTotalFreeSizeForExpHeap(MEMExpHeap *heap)
{
   internal::HeapLock lock(&heap->header);
   auto freeSize = 0u;

   for (auto block = heap->freeList.head; block; block = block->next) {
      freeSize += block->blockSize;
   }

   return freeSize;
}

uint32_t
MEMGetAllocatableSizeForExpHeapEx(MEMExpHeap *heap,
                                  int32_t alignment)
{
   internal::HeapLock lock(&heap->header);
   auto largestFree = 0u;

   if (alignment > 0) {
      decaf_check((alignment & 0x3) == 0);

      for (auto block = heap->freeList.head; block; block = block->next) {
         auto alignedSize = getAlignedBlockSize(block, alignment, MEMExpHeapDirection::FromStart);

         if (alignedSize > largestFree) {
            largestFree = alignedSize;
         }
      }
   } else {
      alignment = -alignment;

      decaf_check((alignment & 0x3) == 0);

      for (auto block = heap->freeList.head; block; block = block->next) {
         auto alignedSize = getAlignedBlockSize(block, alignment, MEMExpHeapDirection::FromEnd);

         if (alignedSize > largestFree) {
            largestFree = alignedSize;
         }
      }
   }

   return largestFree;
}

uint16_t
MEMSetGroupIDForExpHeap(MEMExpHeap *heap,
                        uint16_t id)
{
   internal::HeapLock lock(&heap->header);
   auto originalGroupId = heap->groupId;
   heap->groupId = id;
   return originalGroupId;
}

uint16_t
MEMGetGroupIDForExpHeap(MEMExpHeap *heap)
{
   internal::HeapLock lock(&heap->header);
   return heap->groupId;
}

uint32_t
MEMGetSizeForMBlockExpHeap(uint8_t *addr)
{
   auto block = reinterpret_cast<MEMExpHeapBlock*>(addr - sizeof(MEMExpHeapBlock));
   return block->blockSize;
}

uint16_t
MEMGetGroupIDForMBlockExpHeap(uint8_t *addr)
{
   auto block = reinterpret_cast<MEMExpHeapBlock*>(addr - sizeof(MEMExpHeapBlock));
   return block->attribs.value().groupId();
}

MEMExpHeapDirection
MEMGetAllocDirForMBlockExpHeap(uint8_t *addr)
{
   auto block = reinterpret_cast<MEMExpHeapBlock*>(addr - sizeof(MEMExpHeapBlock));
   return block->attribs.value().allocDir();
}

namespace internal
{

void
dumpExpandedHeap(MEMExpHeap *heap)
{
   internal::HeapLock lock(&heap->header);

   gLog->debug("MEMExpHeap(0x{:8x})", mem::untranslate(heap));
   gLog->debug("Status Address   Size       Group");

   for (auto block = heap->freeList.head; block; block = block->next) {
      auto attribs = static_cast<MEMExpHeapBlockAttribs>(block->attribs);

      gLog->debug("FREE  0x{:8x} 0x{:8x} {:d}",
                  mem::untranslate(block),
                  static_cast<uint32_t>(block->blockSize),
                  attribs.groupId());
   }

   for (auto block = heap->usedList.head; block; block = block->next) {
      auto attribs = static_cast<MEMExpHeapBlockAttribs>(block->attribs);

      gLog->debug("USED  0x{:8x} 0x{:8x} {:d}",
                  mem::untranslate(block),
                  static_cast<uint32_t>(block->blockSize),
                  attribs.groupId());
   }
}

} // namespace internal

void
Module::registerExpHeapFunctions()
{
   RegisterKernelFunction(MEMCreateExpHeapEx);
   RegisterKernelFunction(MEMDestroyExpHeap);
   RegisterKernelFunction(MEMAllocFromExpHeapEx);
   RegisterKernelFunction(MEMFreeToExpHeap);
   RegisterKernelFunction(MEMSetAllocModeForExpHeap);
   RegisterKernelFunction(MEMGetAllocModeForExpHeap);
   RegisterKernelFunction(MEMAdjustExpHeap);
   RegisterKernelFunction(MEMResizeForMBlockExpHeap);
   RegisterKernelFunction(MEMGetTotalFreeSizeForExpHeap);
   RegisterKernelFunction(MEMGetAllocatableSizeForExpHeapEx);
   RegisterKernelFunction(MEMSetGroupIDForExpHeap);
   RegisterKernelFunction(MEMGetGroupIDForExpHeap);
   RegisterKernelFunction(MEMGetSizeForMBlockExpHeap);
   RegisterKernelFunction(MEMGetGroupIDForMBlockExpHeap);
   RegisterKernelFunction(MEMGetAllocDirForMBlockExpHeap);
}

} // namespace coreinit

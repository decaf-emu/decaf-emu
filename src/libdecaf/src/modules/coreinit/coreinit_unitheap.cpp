#include "coreinit.h"
#include "coreinit_unitheap.h"
#include "common/align.h"
#include "common/decaf_assert.h"
#include "common/log.h"

namespace coreinit
{

/**
 * Initialise a unit heap.
 *
 * Adds it to the list of active heaps.
 */
MEMUnitHeap *
MEMCreateUnitHeapEx(MEMUnitHeap *heap,
                    uint32_t size,
                    uint32_t blockSize,
                    int32_t alignment,
                    uint32_t flags)
{
   decaf_check(heap);
   auto start = align_up(mem::untranslate(heap), 4);
   auto end = align_down(mem::untranslate(heap) + size, 4);

   if (start >= end) {
      return nullptr;
   }

   auto firstBlock = align_up<uint32_t>(start + sizeof(MEMUnitHeap), alignment);

   if (firstBlock >= end) {
      return nullptr;
   }

   auto alignedBlockSize = align_up(blockSize, alignment);
   auto blockCount = (end - firstBlock) / alignedBlockSize;

   if (blockCount == 0) {
      return nullptr;
   }

   heap->freeBlocks = mem::translate<MEMUnitHeapFreeBlock>(firstBlock);
   heap->blockSize = alignedBlockSize;

   // Register Heap
   internal::registerHeap(&heap->header,
                          MEMHeapTag::UnitHeap,
                          firstBlock,
                          firstBlock + alignedBlockSize * blockCount,
                          flags);

   // Setup free block linked list
   MEMUnitHeapFreeBlock *prev = nullptr;

   for (auto i = 0u; i < blockCount; ++i) {
      auto block = mem::translate<MEMUnitHeapFreeBlock>(firstBlock + alignedBlockSize * i);

      if (prev) {
         prev->next = block;
      }

      prev = block;
   }

   prev->next = nullptr;
   return heap;
}


/**
 * Destroy unit heap.
 *
 * Remove it from the list of active heaps.
 */
void *
MEMDestroyUnitHeap(MEMUnitHeap *heap)
{
   decaf_check(heap);
   internal::unregisterHeap(&heap->header);
   return heap;
}


/**
 * Allocate a memory block from a unit heap
 */
void *
MEMAllocFromUnitHeap(MEMUnitHeap *heap)
{
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::UnitHeap);

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Acquire(&heap->header.lock);
   }

   auto block = heap->freeBlocks;

   if (block) {
      heap->freeBlocks = block->next;
   }

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Release(&heap->header.lock);
   }

   if (heap->header.flags & MEMHeapFlags::ZeroAllocated) {
      std::memset(block, 0, heap->blockSize);
   } else if (heap->header.flags & MEMHeapFlags::DebugMode) {
      auto value = MEMGetFillValForHeap(MEMHeapFillType::Allocated);
      std::memset(block, value, heap->blockSize);
   }

   return block;
}


/**
 * Free a memory block in a unit heap
 */
void
MEMFreeToUnitHeap(MEMUnitHeap *heap,
                  void *block)
{
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::UnitHeap);

   if (!block) {
      return;
   }

   if (heap->header.flags & MEMHeapFlags::DebugMode) {
      auto value = MEMGetFillValForHeap(MEMHeapFillType::Freed);
      std::memset(block, value, heap->blockSize);
   }

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Acquire(&heap->header.lock);
   }

   auto freeBlock = reinterpret_cast<MEMUnitHeapFreeBlock *>(block);
   freeBlock->next = heap->freeBlocks;
   heap->freeBlocks = freeBlock;

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Release(&heap->header.lock);
   }
}


/**
 * Count the number of free blocks in a unit heap.
 */
uint32_t
MEMCountFreeBlockForUnitHeap(MEMUnitHeap *heap)
{
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::UnitHeap);

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Acquire(&heap->header.lock);
   }

   auto count = 0u;

   for (auto block = heap->freeBlocks; block; block = block->next) {
      count++;
   }

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Release(&heap->header.lock);
   }

   return count;
}


/**
 * Calculate the size required for a unit heap containing blockCount blocks of blockSize.
 */
uint32_t
MEMCalcHeapSizeForUnitHeap(uint32_t blockSize,
                           uint32_t blockCount,
                           int alignment)
{
   auto alignedBlockSize = align_up(blockSize, alignment);
   auto totalBlockSize = alignedBlockSize * blockCount;
   auto headerSize = alignment - 4 + static_cast<uint32_t>(sizeof(MEMUnitHeap));
   return headerSize + totalBlockSize;
}

namespace internal
{

/**
 * Print debug information about the unit heap.
 */
void
dumpUnitHeap(MEMUnitHeap *heap)
{
   auto freeBlocks = MEMCountFreeBlockForUnitHeap(heap);
   auto freeSize = heap->blockSize * freeBlocks;
   auto totalSize = heap->header.dataEnd - heap->header.dataStart;
   auto usedSize = totalSize - freeSize;
   auto percent = static_cast<float>(usedSize) / static_cast<float>(totalSize);

   gLog->debug("MEMUnitHeap(0x{:8x})", mem::untranslate(heap));
   gLog->debug("{} out of {} bytes ({}%) used", usedSize, totalSize, percent);
}

} // namespace internal

void
Module::registerUnitHeapFunctions()
{
   RegisterKernelFunction(MEMCreateUnitHeapEx);
   RegisterKernelFunction(MEMDestroyUnitHeap);
   RegisterKernelFunction(MEMAllocFromUnitHeap);
   RegisterKernelFunction(MEMFreeToUnitHeap);
   RegisterKernelFunction(MEMCountFreeBlockForUnitHeap);
   RegisterKernelFunction(MEMCalcHeapSizeForUnitHeap);
}

} // namespace coreinit

#include "coreinit.h"
#include "coreinit_memunitheap.h"
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
MEMCreateUnitHeapEx(void *base,
                    uint32_t size,
                    uint32_t blockSize,
                    int32_t alignment,
                    uint32_t flags)
{
   decaf_check(base);

   auto baseMem = reinterpret_cast<uint8_t*>(base);

   // Align start and end to 4 byte boundary
   auto start = align_up(baseMem, 4);
   auto end = align_down(baseMem + size, 4);

   if (start >= end) {
      return nullptr;
   }

   // Get first block aligned start
   auto dataStart = align_up(start + sizeof(MEMUnitHeap), alignment);

   if (dataStart >= end) {
      return nullptr;
   }

   // Calculate aligned block size and count
   auto alignedBlockSize = align_up(blockSize, alignment);
   auto blockCount = (end - dataStart) / alignedBlockSize;

   if (blockCount == 0) {
      return nullptr;
   }

   auto heap = reinterpret_cast<MEMUnitHeap*>(start);

   // Register Heap
   internal::registerHeap(&heap->header,
      MEMHeapTag::UnitHeap,
      dataStart,
      dataStart + alignedBlockSize * blockCount,
      flags);

   // Setup the MEMUnitHeap
   auto firstBlock = reinterpret_cast<MEMUnitHeapFreeBlock*>(dataStart);
   heap->freeBlocks = firstBlock;
   heap->blockSize = alignedBlockSize;

   // Setup free block linked list
   MEMUnitHeapFreeBlock *prev = nullptr;

   for (auto i = 0u; i < blockCount; ++i) {
      auto block = firstBlock + alignedBlockSize * i;

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
   decaf_check(heap->header.tag == MEMHeapTag::UnitHeap);
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

   MEMUnitHeapFreeBlock *block = nullptr;

   {
      internal::HeapLock lock(&heap->header);

      block = heap->freeBlocks;

      if (block) {
         heap->freeBlocks = block->next;
      }
   }

   auto heapAttribs = static_cast<MEMHeapAttribs>(heap->header.attribs);

   if (heapAttribs.flags() & MEMHeapFlags::ZeroAllocated) {
      std::memset(block, 0, heap->blockSize);
   } else if (heapAttribs.flags() & MEMHeapFlags::DebugMode) {
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

   auto heapAttribs = static_cast<MEMHeapAttribs>(heap->header.attribs);

   if (heapAttribs.flags() & MEMHeapFlags::DebugMode) {
      auto value = MEMGetFillValForHeap(MEMHeapFillType::Freed);
      std::memset(block, value, heap->blockSize);
   }

   internal::HeapLock lock(&heap->header);

   auto freeBlock = reinterpret_cast<MEMUnitHeapFreeBlock *>(block);
   freeBlock->next = heap->freeBlocks;
   heap->freeBlocks = freeBlock;
}


/**
 * Count the number of free blocks in a unit heap.
 */
uint32_t
MEMCountFreeBlockForUnitHeap(MEMUnitHeap *heap)
{
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::UnitHeap);

   internal::HeapLock lock(&heap->header);

   auto count = 0u;

   for (auto block = heap->freeBlocks; block; block = block->next) {
      count++;
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
   auto totalSize = heap->header.dataEnd.getAddress() - heap->header.dataStart.getAddress();
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

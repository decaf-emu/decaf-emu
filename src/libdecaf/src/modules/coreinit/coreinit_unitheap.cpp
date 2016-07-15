#include "coreinit.h"
#include "coreinit_unitheap.h"
#include "common/align.h"
#include "common/log.h"

namespace coreinit
{

#pragma pack(push, 1)

struct UnitBlock
{
   virtual_ptr<UnitBlock> next;
};

struct UnitHeap : MEMHeapHeader
{
   virtual_ptr<UnitBlock> freeBlockList;
   uint32_t blockSize;
};

#pragma pack(pop)


/**
 * Initialise a unit heap.
 *
 * Adds it to the list of active heaps.
 */
UnitHeap *
MEMCreateUnitHeapEx(UnitHeap *heap,
                    uint32_t size,
                    uint32_t blockSize,
                    int32_t alignment,
                    uint16_t flags)
{
   // Adjust block size
   auto adjBlockSize = static_cast<uint32_t>(align_up(blockSize + sizeof(UnitBlock), alignment));

   // Calculate blocks
   auto base = mem::untranslate(heap);
   auto firstBlock = static_cast<uint32_t>(align_up(base + sizeof(UnitHeap) + sizeof(UnitBlock), alignment) - sizeof(UnitBlock));
   auto blockCount = (size - (firstBlock - base)) / adjBlockSize;

   // Setup unit heap
   heap->blockSize = blockSize;

   if (blockCount == 0) {
      heap->freeBlockList = nullptr;
   } else {
      heap->freeBlockList = make_virtual_ptr<UnitBlock>(firstBlock);
   }

   // Register heap
   internal::registerHeap(heap,
                          MEMHeapTag::UnitHeap,
                          firstBlock,
                          firstBlock + adjBlockSize * blockCount,
                          flags);

   // Setup free block list
   for (auto i = 0u; i < blockCount; ++i) {
      auto block = make_virtual_ptr<UnitBlock>(firstBlock + adjBlockSize * i);

      if (blockCount == (i + 1)) {
         block->next = nullptr;
      } else {
         block->next = make_virtual_ptr<UnitBlock>(firstBlock + adjBlockSize * (i + 1));
      }
   }

   return heap;
}


/**
 * Destroy unit heap.
 *
 * Remove it from the list of active heaps.
 */
void *
MEMDestroyUnitHeap(UnitHeap *heap)
{
   internal::unregisterHeap(heap);
   return heap;
}


/**
 * Allocate a memory block from a unit heap
 */
void *
MEMAllocFromUnitHeap(UnitHeap *heap)
{
   ScopedSpinLock lock(&heap->lock);
   auto freeBlock = heap->freeBlockList;
   void *block = nullptr;

   if (freeBlock) {
      heap->freeBlockList = freeBlock->next;
      block = make_virtual_ptr<void>(freeBlock.getAddress() + sizeof(UnitBlock));
   }

   return block;
}


/**
 * Free a memory block in a unit heap
 */
void
MEMFreeToUnitHeap(UnitHeap *heap, void *block)
{
   ScopedSpinLock lock(&heap->lock);
   auto blockAddr = mem::untranslate(block);
   auto unitBlock = make_virtual_ptr<UnitBlock>(blockAddr - sizeof(UnitBlock));
   unitBlock->next = heap->freeBlockList;
   heap->freeBlockList = unitBlock;
}


/**
 * Print debug information about the unit heap.
 */
void
MEMiDumpUnitHeap(UnitHeap *heap)
{
   auto freeBlocks = MEMCountFreeBlockForUnitHeap(heap);
   auto freeSize = heap->blockSize * freeBlocks;
   auto totalSize = heap->dataEnd - heap->dataStart;
   auto usedSize = totalSize - freeSize;
   auto percent = static_cast<float>(usedSize) / static_cast<float>(totalSize);

   gLog->debug("MEMiDumpUnitHeap({:8x})", mem::untranslate(heap));
   gLog->debug("{} out of {} bytes ({}%) used", usedSize, totalSize, percent);
}


/**
 * Count the number of free blocks in a unit heap.
 */
uint32_t
MEMCountFreeBlockForUnitHeap(UnitHeap *heap)
{
   ScopedSpinLock lock(&heap->lock);
   auto count = 0u;

   for (auto block = heap->freeBlockList; block; block = block->next) {
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
   auto adjBlockSize = static_cast<uint32_t>(align_up(blockSize + sizeof(UnitBlock), alignment));
   auto firstBlock = static_cast<uint32_t>(align_up(sizeof(UnitHeap) + sizeof(UnitBlock), alignment) - sizeof(UnitBlock));

   return firstBlock + (adjBlockSize * blockCount);
}


void
Module::registerUnitHeapFunctions()
{
   RegisterKernelFunction(MEMCreateUnitHeapEx);
   RegisterKernelFunction(MEMDestroyUnitHeap);
   RegisterKernelFunction(MEMAllocFromUnitHeap);
   RegisterKernelFunction(MEMFreeToUnitHeap);
   RegisterKernelFunction(MEMiDumpUnitHeap);
   RegisterKernelFunction(MEMCountFreeBlockForUnitHeap);
   RegisterKernelFunction(MEMCalcHeapSizeForUnitHeap);
}

} // namespace coreinit

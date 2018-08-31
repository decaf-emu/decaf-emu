#include "coreinit.h"
#include "coreinit_memunitheap.h"
#include "coreinit_memory.h"

#include <common/align.h>
#include <common/decaf_assert.h>
#include <common/log.h>

namespace cafe::coreinit
{

/**
 * Initialise a unit heap.
 *
 * Adds it to the list of active heaps.
 */
MEMHeapHandle
MEMCreateUnitHeapEx(virt_ptr<void> base,
                    uint32_t size,
                    uint32_t blockSize,
                    int32_t alignment,
                    uint32_t flags)
{
   decaf_check(base);

   auto baseMem = virt_cast<uint8_t *>(base);

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

   auto heap = virt_cast<MEMUnitHeap *>(start);

   // Register Heap
   internal::registerHeap(virt_addrof(heap->header),
                          MEMHeapTag::UnitHeap,
                          dataStart,
                          dataStart + alignedBlockSize * blockCount,
                          static_cast<MEMHeapFlags>(flags));

   // Setup the MEMUnitHeap
   auto firstBlock = virt_cast<MEMUnitHeapFreeBlock *>(dataStart);
   heap->freeBlocks = firstBlock;
   heap->blockSize = alignedBlockSize;

   // Setup free block linked list
   auto prev = virt_ptr<MEMUnitHeapFreeBlock> { nullptr };

   for (auto i = 0u; i < blockCount; ++i) {
      auto block = virt_cast<MEMUnitHeapFreeBlock *>(dataStart + alignedBlockSize * i);

      if (prev) {
         prev->next = block;
      }

      prev = block;
   }

   if (prev) {
      prev->next = nullptr;
   }

   return virt_cast<MEMHeapHeader *>(heap);
}


/**
 * Destroy unit heap.
 *
 * Remove it from the list of active heaps.
 */
virt_ptr<void>
MEMDestroyUnitHeap(MEMHeapHandle handle)
{
   auto heap = virt_cast<MEMUnitHeap *>(handle);
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::UnitHeap);
   internal::unregisterHeap(virt_addrof(heap->header));
   return heap;
}


/**
 * Allocate a memory block from a unit heap
 */
virt_ptr<void>
MEMAllocFromUnitHeap(MEMHeapHandle handle)
{
   auto heap = virt_cast<MEMUnitHeap *>(handle);
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::UnitHeap);

   internal::HeapLock lock { virt_addrof(heap->header) };
   auto block = heap->freeBlocks;

   if (block) {
      heap->freeBlocks = block->next;
   }

   lock.unlock();

   if (block) {
      if (heap->header.flags & MEMHeapFlags::ZeroAllocated) {
         memset(block, 0, heap->blockSize);
      } else if (heap->header.flags & MEMHeapFlags::DebugMode) {
         auto value = MEMGetFillValForHeap(MEMHeapFillType::Allocated);
         memset(block, value, heap->blockSize);
      }
   }

   return block;
}


/**
 * Free a memory block in a unit heap
 */
void
MEMFreeToUnitHeap(MEMHeapHandle handle,
                  virt_ptr<void> block)
{
   auto heap = virt_cast<MEMUnitHeap *>(handle);
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::UnitHeap);

   if (!block) {
      return;
   }

   if (heap->header.flags & MEMHeapFlags::DebugMode) {
      auto value = MEMGetFillValForHeap(MEMHeapFillType::Freed);
      memset(block, value, heap->blockSize);
   }

   internal::HeapLock lock { virt_addrof(heap->header) };
   auto freeBlock = virt_cast<MEMUnitHeapFreeBlock *>(block);
   freeBlock->next = heap->freeBlocks;
   heap->freeBlocks = freeBlock;
}


/**
 * Count the number of free blocks in a unit heap.
 */
uint32_t
MEMCountFreeBlockForUnitHeap(MEMHeapHandle handle)
{
   auto heap = virt_cast<MEMUnitHeap *>(handle);
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::UnitHeap);

   internal::HeapLock lock { virt_addrof(heap->header) };
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
dumpUnitHeap(virt_ptr<MEMUnitHeap> heap)
{
   auto handle = virt_cast<MEMHeapHeader *>(heap);
   auto freeBlocks = MEMCountFreeBlockForUnitHeap(handle);
   auto freeSize = heap->blockSize * freeBlocks;
   auto totalSize = heap->header.dataEnd - heap->header.dataStart;
   auto usedSize = totalSize - freeSize;
   auto percent = static_cast<float>(usedSize) / static_cast<float>(totalSize);

   gLog->debug("MEMUnitHeap(0x{:8x})", heap);
   gLog->debug("{} out of {} bytes ({}%) used", usedSize, totalSize, percent);
}

} // namespace internal

void
Library::registerMemUnitHeapSymbols()
{
   RegisterFunctionExport(MEMCreateUnitHeapEx);
   RegisterFunctionExport(MEMDestroyUnitHeap);
   RegisterFunctionExport(MEMAllocFromUnitHeap);
   RegisterFunctionExport(MEMFreeToUnitHeap);
   RegisterFunctionExport(MEMCountFreeBlockForUnitHeap);
   RegisterFunctionExport(MEMCalcHeapSizeForUnitHeap);
}

} // namespace cafe::coreinit

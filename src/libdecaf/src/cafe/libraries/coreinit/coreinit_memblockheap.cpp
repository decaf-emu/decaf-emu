#include "coreinit.h"
#include "coreinit_memheap.h"
#include "coreinit_memblockheap.h"
#include "coreinit_memory.h"

#include <common/align.h>
#include <common/log.h>

namespace cafe::coreinit
{

/**
 * Initialise block heap.
 */
MEMHeapHandle
MEMInitBlockHeap(virt_ptr<void> base,
                 virt_ptr<void> start,
                 virt_ptr<void> end,
                 virt_ptr<MEMBlockHeapTracking> tracking,
                 uint32_t size,
                 uint32_t flags)
{
   auto heap = virt_cast<MEMBlockHeap *>(base);
   if (!heap || !start || !end || start >= end) {
      return nullptr;
   }

   auto dataStart = virt_cast<uint8_t *>(start);
   auto dataEnd = virt_cast<uint8_t *>(end);

   // Register heap
   internal::registerHeap(virt_addrof(heap->header),
                          MEMHeapTag::BlockHeap,
                          dataStart,
                          dataEnd,
                          static_cast<MEMHeapFlags>(flags));

   // Setup default tracker
   heap->defaultTrack.blockCount = 1u;
   heap->defaultTrack.blocks = virt_addrof(heap->defaultBlock);

   // Setup default block
   heap->defaultBlock.start = dataStart;
   heap->defaultBlock.end = dataEnd;
   heap->defaultBlock.isFree = TRUE;
   heap->defaultBlock.next = nullptr;
   heap->defaultBlock.prev = nullptr;

   // Add default block to block list
   heap->firstBlock = virt_addrof(heap->defaultBlock);
   heap->lastBlock = virt_addrof(heap->defaultBlock);

   auto handle = virt_cast<MEMHeapHeader *>(heap);
   MEMAddBlockHeapTracking(handle, tracking, size);
   return handle;
}


/**
 * Destroy block heap.
 */
virt_ptr<void>
MEMDestroyBlockHeap(MEMHeapHandle handle)
{
   auto heap = virt_cast<MEMBlockHeap *>(handle);
   if (!heap || heap->header.tag != MEMHeapTag::BlockHeap) {
      return nullptr;
   }

   memset(heap, 0, sizeof(MEMBlockHeap));
   internal::unregisterHeap(virt_addrof(heap->header));
   return heap;
}


/**
 * Adds more tracking block memory to block heap.
 */
int
MEMAddBlockHeapTracking(MEMHeapHandle handle,
                        virt_ptr<MEMBlockHeapTracking> tracking,
                        uint32_t size)
{
   auto heap = virt_cast<MEMBlockHeap *>(handle);
   if (!heap || !tracking || heap->header.tag != MEMHeapTag::BlockHeap) {
      return -4;
   }

   // Size must be enough to contain at least 1 tracking structure and 1 block
   if (size < sizeof(MEMBlockHeapTracking) + sizeof(MEMBlockHeapBlock)) {
      return -4;
   }

   auto blockCount = static_cast<uint32_t>((size - sizeof(MEMBlockHeapTracking)) / sizeof(MEMBlockHeapBlock));
   auto blocks = virt_cast<MEMBlockHeapBlock *>(tracking + 1);

   // Setup tracking data
   tracking->blockCount = blockCount;
   tracking->blocks = blocks;

   // Setup block linked list
   for (auto i = 0u; i < blockCount; ++i) {
      auto &block = blocks[i];
      block.prev = nullptr;
      block.next = virt_addrof(blocks[i + 1]);
   }

   // Insert at start of block list
   internal::HeapLock lock { virt_addrof(heap->header) };
   blocks[blockCount - 1].next = heap->firstFreeBlock;
   heap->firstFreeBlock = blocks;
   heap->numFreeBlocks += blockCount;

   return 0;
}

static virt_ptr<MEMBlockHeapBlock>
findBlockOwning(virt_ptr<MEMBlockHeap> heap,
                virt_ptr<void> data)
{
   auto addr = virt_cast<uint8_t *>(data);

   if (addr < heap->header.dataStart) {
      return nullptr;
   }

   if (addr >= heap->header.dataEnd) {
      return nullptr;
   }

   auto distFromEnd = heap->header.dataEnd - addr;
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
allocInsideBlock(virt_ptr<MEMBlockHeap> heap,
                 virt_ptr<MEMBlockHeapBlock> block,
                 virt_ptr<uint8_t> start,
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
   if (heap->header.flags & MEMHeapFlags::ZeroAllocated) {
      memset(block->start, 0, size);
   } else if (heap->header.flags & MEMHeapFlags::DebugMode) {
      auto value = MEMGetFillValForHeap(MEMHeapFillType::Allocated);
      memset(block->start, value, size);
   }

   // Set block to allocated and return success!
   block->isFree = FALSE;
   return true;
}


/**
 * Try to allocate from block heap at a specific address.
 */
virt_ptr<void>
MEMAllocFromBlockHeapAt(MEMHeapHandle handle,
                        virt_ptr<void> addr,
                        uint32_t size)
{
   auto heap = virt_cast<MEMBlockHeap *>(handle);
   if (!heap || !addr || !size || heap->header.tag != MEMHeapTag::BlockHeap) {
      return nullptr;
   }

   if (!heap->firstFreeBlock) {
      return nullptr;
   }

   internal::HeapLock lock { virt_addrof(heap->header) };

   auto block = findBlockOwning(heap, addr);

   if (!block) {
      gLog->warn("MEMAllocFromBlockHeapAt: Could not find block containing addr 0x{:08X}",
                 addr);
      return nullptr;
   }

   if (!block->isFree) {
      gLog->warn("MEMAllocFromBlockHeapAt: Requested address is not free 0x{:08X}",
                 addr);
      return nullptr;
   }

   if (!allocInsideBlock(heap, block, virt_cast<uint8_t *>(addr), size)) {
      return nullptr;
   }

   return addr;
}


/**
 * Allocate from block heap.
 */
virt_ptr<void>
MEMAllocFromBlockHeapEx(MEMHeapHandle handle,
                        uint32_t size,
                        int32_t align)
{
   auto heap = virt_cast<MEMBlockHeap *>(handle);
   auto block = virt_ptr<MEMBlockHeapBlock> { nullptr };
   auto alignedStart = virt_ptr<uint8_t> { nullptr };
   auto result = virt_ptr<void> { nullptr };

   if (!heap || !size || heap->header.tag != MEMHeapTag::BlockHeap) {
      return nullptr;
   }

   internal::HeapLock lock { virt_addrof(heap->header) };

   if (align == 0) {
      align = 4;
   }

   if (align >= 0) {
      // Find first free block with enough size
      for (block = heap->firstBlock; block; block = block->next) {
         if (block->isFree) {
            alignedStart = align_up(block->start, align);

            if (alignedStart + size < block->end) {
               break;
            }
         }
      }
   } else {
      // Find last free block with enough size
      for (block = heap->lastBlock; block; block = block->prev) {
         if (block->isFree) {
            alignedStart = align_down(block->end - size, -align);

            if (alignedStart >= block->start) {
               break;
            }
         }
      }
   }

   if (!block) {
      gLog->warn("MEMAllocFromBlockHeapEx: Could not find free block size: 0x{:X} align: 0x{:X}, allocatable: 0x{:X} free: 0x{:X}",
                 size,
                 align,
                 MEMGetAllocatableSizeForBlockHeapEx(virt_cast<MEMHeapHeader *>(heap), align),
                 MEMGetTotalFreeSizeForBlockHeap(virt_cast<MEMHeapHeader *>(heap)));
   } else if (allocInsideBlock(heap, block, alignedStart, size)) {
      result = alignedStart;
   }

   return result;
}


/**
 * Free memory back to block heap.
 */
void
MEMFreeToBlockHeap(MEMHeapHandle handle,
                   virt_ptr<void> data)
{
   auto heap = virt_cast<MEMBlockHeap *>(handle);
   if (!heap || !data || heap->header.tag != MEMHeapTag::BlockHeap) {
      return;
   }

   internal::HeapLock lock { virt_addrof(heap->header) };
   auto block = findBlockOwning(heap, data);

   if (!block) {
      gLog->warn("MEMFreeToBlockHeap: Could not find block containing data 0x{:08X}",
                 data);
      return;
   }

   if (block->isFree) {
      gLog->warn("MEMFreeToBlockHeap: Tried to free an already free block");
      return;
   }

   if (block->start != data) {
      gLog->warn("MEMFreeToBlockHeap: Tried to free block 0x{:08X} from middle 0x{:08X}",
                 block->start, data);
      return;
   }

   if (heap->header.flags & MEMHeapFlags::DebugMode) {
      auto fill = MEMGetFillValForHeap(MEMHeapFillType::Freed);
      auto size = block->end - block->start;
      std::memset(block->start.getRawPointer(), fill, size);
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
MEMGetAllocatableSizeForBlockHeapEx(MEMHeapHandle handle,
                                    int32_t align)
{
   auto heap = virt_cast<MEMBlockHeap *>(handle);
   if (!heap || heap->header.tag != MEMHeapTag::BlockHeap) {
      return 0;
   }

   internal::HeapLock lock { virt_addrof(heap->header) };
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
      auto startAddr = block->start;
      auto endAddr = block->end;
      auto alignedStart = align_up(startAddr, align);

      if (alignedStart >= endAddr) {
         continue;
      }

      // See if this block is largest free block so far
      auto freeSize = static_cast<uint32_t>(endAddr - alignedStart);

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
MEMGetTrackingLeftInBlockHeap(MEMHeapHandle handle)
{
   auto heap = virt_cast<MEMBlockHeap *>(handle);
   if (!heap || heap->header.tag != MEMHeapTag::BlockHeap) {
      return 0;
   }

   return heap->numFreeBlocks;
}

/**
 * Return total free size in the heap.
 */
uint32_t
MEMGetTotalFreeSizeForBlockHeap(MEMHeapHandle handle)
{
   auto heap = virt_cast<MEMBlockHeap *>(handle);
   if (!heap || heap->header.tag != MEMHeapTag::BlockHeap) {
      return 0;
   }

   internal::HeapLock lock { virt_addrof(heap->header) };
   auto freeSize = 0u;

   for (auto block = heap->firstBlock; block; block = block->next) {
      if (!block->isFree) {
         continue;
      }

      auto startAddr = block->start;
      auto endAddr = block->end;
      freeSize += static_cast<uint32_t>(endAddr - startAddr);
   }

   return freeSize;
}

void
Library::registerMemBlockHeapSymbols()
{
   RegisterFunctionExport(MEMInitBlockHeap);
   RegisterFunctionExport(MEMDestroyBlockHeap);
   RegisterFunctionExport(MEMAddBlockHeapTracking);
   RegisterFunctionExport(MEMAllocFromBlockHeapAt);
   RegisterFunctionExport(MEMAllocFromBlockHeapEx);
   RegisterFunctionExport(MEMFreeToBlockHeap);
   RegisterFunctionExport(MEMGetAllocatableSizeForBlockHeapEx);
   RegisterFunctionExport(MEMGetTrackingLeftInBlockHeap);
   RegisterFunctionExport(MEMGetTotalFreeSizeForBlockHeap);
}

} // cafe::coreinit

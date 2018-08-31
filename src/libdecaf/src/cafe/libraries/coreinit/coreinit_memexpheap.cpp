#include "coreinit.h"
#include "coreinit_memexpheap.h"
#include "coreinit_memory.h"
#include <common/log.h>

namespace cafe::coreinit
{

static constexpr auto
FreeTag = uint16_t { 0x4654 }; // 'FR'

static constexpr auto
UsedTag = uint16_t { 0x5544 }; // 'UD'

static virt_ptr<uint8_t>
getBlockMemStart(virt_ptr<MEMExpHeapBlock> block)
{
   auto attribs = block->attribs.value();
   return virt_cast<uint8_t *>(block) - attribs.alignment();
}

static virt_ptr<uint8_t>
getBlockMemEnd(virt_ptr<MEMExpHeapBlock> block)
{
   return virt_cast<uint8_t *>(block) + sizeof(MEMExpHeapBlock) + block->blockSize;
}

static virt_ptr<uint8_t>
getBlockDataStart(virt_ptr<MEMExpHeapBlock> block)
{
   return virt_cast<uint8_t *>(block) + sizeof(MEMExpHeapBlock);
}

static virt_ptr<MEMExpHeapBlock>
getUsedMemBlock(virt_ptr<void> mem)
{
   auto block = virt_cast<MEMExpHeapBlock *>(mem) - 1;
   decaf_check(block->tag == UsedTag);
   return block;
}

static bool
listContainsBlock(virt_ptr<MEMExpHeapBlockList> list,
                  virt_ptr<MEMExpHeapBlock> block)
{
   for (auto i = list->head; i; i = i->next) {
      if (i == block) {
         return true;
      }
   }

   return false;
}

static void
insertBlock(virt_ptr<MEMExpHeapBlockList> list,
            virt_ptr<MEMExpHeapBlock> prev,
            virt_ptr<MEMExpHeapBlock> block)
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
removeBlock(virt_ptr<MEMExpHeapBlockList> list,
            virt_ptr<MEMExpHeapBlock> block)
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
getAlignedBlockSize(virt_ptr<MEMExpHeapBlock> block,
                    uint32_t alignment,
                    MEMExpHeapDirection dir)
{
   if (dir == MEMExpHeapDirection::FromStart) {
      auto dataStart = virt_cast<uint8_t *>(block) + sizeof(MEMExpHeapBlock);
      auto dataEnd = dataStart + block->blockSize;
      auto alignedDataStart = align_up(dataStart, alignment);

      if (alignedDataStart >= dataEnd) {
         return 0;
      }

      return static_cast<uint32_t>(dataEnd - alignedDataStart);
   } else if (dir == MEMExpHeapDirection::FromEnd) {
      auto dataStart = virt_cast<uint8_t *>(block) + sizeof(MEMExpHeapBlock);
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

static virt_ptr<MEMExpHeapBlock>
createUsedBlockFromFreeBlock(virt_ptr<MEMExpHeap> heap,
                             virt_ptr<MEMExpHeapBlock> freeBlock,
                             uint32_t size,
                             uint32_t alignment,
                             MEMExpHeapDirection dir)
{
   auto expHeapAttribs = heap->attribs.value();
   auto freeBlockAttribs = freeBlock->attribs.value();

   auto freeBlockPrev = freeBlock->prev;
   auto freeMemStart = getBlockMemStart(freeBlock);
   auto freeMemEnd = getBlockMemEnd(freeBlock);

   // Free blocks should never have alignment...
   decaf_check(!freeBlockAttribs.alignment());
   removeBlock(virt_addrof(heap->freeList), freeBlock);

   // Find where we are going to start
   auto alignedDataStart = virt_ptr<uint8_t> { };

   if (dir == MEMExpHeapDirection::FromStart) {
      alignedDataStart = align_up(freeMemStart + sizeof(MEMExpHeapBlock), alignment);
   } else if (dir == MEMExpHeapDirection::FromEnd) {
      alignedDataStart = align_down(freeMemEnd - size, alignment);
   } else {
      decaf_abort("Unexpected ExpHeap direction");
   }

   // Grab the block header pointer and validate everything is sane
   auto alignedBlock = virt_cast<MEMExpHeapBlock *>(alignedDataStart) - 1;
   decaf_check(alignedDataStart - sizeof(MEMExpHeapBlock) >= freeMemStart);
   decaf_check(alignedDataStart + size <= freeMemEnd);

   // Calculate the alignment waste
   auto topSpaceRemain = (alignedDataStart - freeMemStart) - sizeof(MEMExpHeapBlock);
   auto bottomSpaceRemain = static_cast<uint32_t>((freeMemEnd - alignedDataStart) - size);

   if (expHeapAttribs.reuseAlignSpace() || dir == MEMExpHeapDirection::FromEnd) {
      // If the user wants to reuse the alignment space, or we allocated from the bottom,
      //  we should try to release the top space back to the heap free list.
      if (topSpaceRemain > sizeof(MEMExpHeapBlock) + 4) {
         // We have enough room to put some of the memory back to the free list
         freeBlock = virt_cast<MEMExpHeapBlock *>(freeMemStart);
         freeBlock->attribs = MEMExpHeapBlockAttribs::get(0);
         freeBlock->blockSize = static_cast<uint32_t>(topSpaceRemain - sizeof(MEMExpHeapBlock));
         freeBlock->next = nullptr;
         freeBlock->prev = nullptr;
         freeBlock->tag = FreeTag;

         insertBlock(virt_addrof(heap->freeList), freeBlockPrev, freeBlock);
         topSpaceRemain = 0;
      }
   }

   if (expHeapAttribs.reuseAlignSpace() || dir == MEMExpHeapDirection::FromStart) {
      // If the user wants to reuse the alignment space, or we allocated from the top,
      //  we should try to release the bottom space back to the heap free list.
      if (bottomSpaceRemain > sizeof(MEMExpHeapBlock) + 4) {
         // We have enough room to put some of the memory back to the free list
         freeBlock = virt_cast<MEMExpHeapBlock *>(freeMemEnd - bottomSpaceRemain);
         freeBlock->attribs = MEMExpHeapBlockAttribs::get(0);
         freeBlock->blockSize = static_cast<uint32_t>(bottomSpaceRemain - sizeof(MEMExpHeapBlock));
         freeBlock->next = nullptr;
         freeBlock->prev = nullptr;
         freeBlock->tag = FreeTag;

         insertBlock(virt_addrof(heap->freeList), freeBlockPrev, freeBlock);
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

   insertBlock(virt_addrof(heap->usedList), nullptr, alignedBlock);

   if (heap->header.flags & MEMHeapFlags::ZeroAllocated) {
      memset(alignedDataStart, 0, size);
   } else if (heap->header.flags & MEMHeapFlags::DebugMode) {
      auto fillVal = MEMGetFillValForHeap(MEMHeapFillType::Allocated);
      memset(alignedDataStart, fillVal, size);
   }

   return alignedBlock;
}

static void
releaseMemory(virt_ptr<MEMExpHeap> heap,
              virt_ptr<uint8_t> memStart,
              virt_ptr<uint8_t> memEnd)
{
   decaf_check(memEnd - memStart >= sizeof(MEMExpHeapBlock) + 4);

   // Fill the released memory with debug data if needed
   if (heap->header.flags & MEMHeapFlags::DebugMode) {
      auto fillVal = MEMGetFillValForHeap(MEMHeapFillType::Freed);
      std::memset(memStart.getRawPointer(), fillVal, memEnd - memStart);
   }

   // Find the preceeding block to the memory we are releasing
   virt_ptr<MEMExpHeapBlock> prevBlock = nullptr;
   virt_ptr<MEMExpHeapBlock> nextBlock = heap->freeList.head;

   for (auto block = heap->freeList.head; block; block = block->next) {
      if (getBlockMemStart(block) < memStart) {
         prevBlock = block;
         nextBlock = block->next;
      } else if (block >= prevBlock) {
         break;
      }
   }

   virt_ptr<MEMExpHeapBlock> freeBlock = nullptr;
   if (prevBlock) {
      // If there is a previous block, we need to check if we
      //  should just steal that block rather than making one.
      auto prevMemEnd = getBlockMemEnd(prevBlock);

      if (memStart == prevMemEnd) {
         // Previous block absorbs the new memory
         prevBlock->blockSize += static_cast<uint32_t>(memEnd - memStart);

         // Our free block becomes the previous one
         freeBlock = prevBlock;
      }
   }

   if (!freeBlock) {
      // We did not steal the previous block to free into,
      //  we need to allocate our own here.
      freeBlock = virt_cast<MEMExpHeapBlock *>(memStart);
      freeBlock->attribs = MEMExpHeapBlockAttribs::get(0);
      freeBlock->blockSize = static_cast<uint32_t>((memEnd - memStart) - sizeof(MEMExpHeapBlock));
      freeBlock->next = nullptr;
      freeBlock->prev = nullptr;
      freeBlock->tag = FreeTag;

      insertBlock(virt_addrof(heap->freeList), prevBlock, freeBlock);
   }

   if (nextBlock) {
      // If there is a next block, we need to possibly merge it down
      //  into this one.
      auto nextBlockStart = getBlockMemStart(nextBlock);

      if (nextBlockStart == memEnd) {
         // The next block needs to be merged into the freeBlock, as they
         //  are directly adjacent to each other in memory.
         auto nextBlockEnd = getBlockMemEnd(nextBlock);
         freeBlock->blockSize += static_cast<uint32_t>(nextBlockEnd - nextBlockStart);

         removeBlock(virt_addrof(heap->freeList), nextBlock);
      }
   }
}

MEMHeapHandle
MEMCreateExpHeapEx(virt_ptr<void> base,
                   uint32_t size,
                   uint32_t flags)
{
   decaf_check(base);

   auto heapData = virt_cast<uint8_t *>(base);
   auto alignedStart = align_up(heapData, 4);
   auto alignedEnd = align_down(heapData + size, 4);

   if (alignedEnd < alignedStart || alignedEnd - alignedStart < 0x6C) {
      // Not enough room for the header
      return nullptr;
   }

   // Get our heap header
   auto heap = virt_cast<MEMExpHeap *>(alignedStart);

   // Register Heap
   internal::registerHeap(virt_addrof(heap->header),
                          MEMHeapTag::ExpandedHeap,
                          alignedStart + sizeof(MEMExpHeap),
                          alignedEnd,
                          static_cast<MEMHeapFlags>(flags));

   // Create an initial block of the data
   auto dataStart = alignedStart + sizeof(MEMExpHeap);
   auto firstBlock = virt_cast<MEMExpHeapBlock *>(dataStart);

   firstBlock->attribs = MEMExpHeapBlockAttribs::get(0);
   firstBlock->blockSize = static_cast<uint32_t>((alignedEnd - dataStart) - sizeof(MEMExpHeapBlock));
   firstBlock->next = nullptr;
   firstBlock->prev = nullptr;
   firstBlock->tag = FreeTag;

   heap->freeList.head = firstBlock;
   heap->freeList.tail = firstBlock;
   heap->usedList.head = nullptr;
   heap->usedList.tail = nullptr;

   heap->groupId = uint16_t { 0 };
   heap->attribs = MEMExpHeapAttribs::get(0);

   return virt_cast<MEMHeapHeader *>(heap);
}

virt_ptr<void>
MEMDestroyExpHeap(MEMHeapHandle handle)
{
   auto heap = virt_cast<MEMExpHeap *>(handle);
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::ExpandedHeap);
   internal::unregisterHeap(virt_addrof(heap->header));
   return heap;
}

virt_ptr<void>
MEMAllocFromExpHeapEx(MEMHeapHandle handle,
                      uint32_t size,
                      int32_t alignment)
{
   auto heap = virt_cast<MEMExpHeap *>(handle);
   decaf_check(heap->header.tag == MEMHeapTag::ExpandedHeap);
   auto expHeapFlags = heap->attribs.value();

   if (size == 0) {
      size = 1;
   }

   decaf_check(alignment != 0);

   internal::HeapLock lock { virt_addrof(heap->header) };
   virt_ptr<MEMExpHeapBlock> newBlock = nullptr;

   size = align_up(size, 4);

   if (alignment > 0) {
      auto foundBlock = virt_ptr<MEMExpHeapBlock> { nullptr };
      auto bestAlignedSize = 0xFFFFFFFFu;

      alignment = std::max(4, alignment);
      decaf_check((alignment & 0x3) == 0);

      for (auto block = heap->freeList.head; block; block = block->next) {
         auto alignedSize = getAlignedBlockSize(block,
                                                alignment,
                                                MEMExpHeapDirection::FromStart);

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
         newBlock = createUsedBlockFromFreeBlock(heap,
                                                 foundBlock,
                                                 size,
                                                 alignment,
                                                 MEMExpHeapDirection::FromStart);
      }
   } else {
      alignment = std::max(4, -alignment);
      decaf_check((alignment & 0x3) == 0);

      auto foundBlock = virt_ptr<MEMExpHeapBlock> { nullptr };
      auto bestAlignedSize = 0xFFFFFFFFu;

      for (auto block = heap->freeList.head; block; block = block->next) {
         auto alignedSize = getAlignedBlockSize(block,
                                                alignment,
                                                MEMExpHeapDirection::FromEnd);

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
         newBlock = createUsedBlockFromFreeBlock(heap,
                                                 foundBlock,
                                                 size,
                                                 alignment,
                                                 MEMExpHeapDirection::FromEnd);
      }
   }

   if (!newBlock) {
      MEMDumpHeap(virt_addrof(heap->header));
      return nullptr;
   }

   return getBlockDataStart(newBlock);
}

void
MEMFreeToExpHeap(MEMHeapHandle handle,
                 virt_ptr<void> mem)
{
   auto heap = virt_cast<MEMExpHeap *>(handle);
   decaf_check(heap->header.tag == MEMHeapTag::ExpandedHeap);

   if (!mem) {
      return;
   }

   internal::HeapLock lock { virt_addrof(heap->header) };

   // Find the block
   auto dataStart = virt_cast<uint8_t *>(mem);
   auto block = virt_cast<MEMExpHeapBlock *>(dataStart - sizeof(MEMExpHeapBlock));

   // Get the bounding region for this block
   auto memStart = getBlockMemStart(block);
   auto memEnd = getBlockMemEnd(block);

   // Remove the block from the used list
   removeBlock(virt_addrof(heap->usedList), block);

   // Release the memory back to the heap free list
   releaseMemory(heap, memStart, memEnd);
}

MEMExpHeapMode
MEMSetAllocModeForExpHeap(MEMHeapHandle handle,
                          MEMExpHeapMode mode)
{
   auto heap = virt_cast<MEMExpHeap *>(handle);
   internal::HeapLock lock { virt_addrof(heap->header) };

   auto expHeapAttribs = heap->attribs.value();
   heap->attribs = expHeapAttribs.allocMode(mode);
   return expHeapAttribs.allocMode();
}

MEMExpHeapMode
MEMGetAllocModeForExpHeap(MEMHeapHandle handle)
{
   auto heap = virt_cast<MEMExpHeap *>(handle);
   internal::HeapLock lock { virt_addrof(heap->header) };

   auto expHeapAttribs = heap->attribs.value();
   return expHeapAttribs.allocMode();
}

uint32_t
MEMAdjustExpHeap(MEMHeapHandle handle)
{
   auto heap = virt_cast<MEMExpHeap *>(handle);
   internal::HeapLock lock { virt_addrof(heap->header) };
   auto lastFreeBlock = heap->freeList.tail;

   if (!lastFreeBlock) {
      return 0;
   }

   auto blockData = virt_cast<uint8_t *>(lastFreeBlock) + sizeof(MEMExpHeapBlock);

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

   auto heapMemStart = virt_cast<uint8_t *>(heap);
   auto heapMemEnd = virt_cast<uint8_t *>(heap->header.dataEnd);
   return static_cast<uint32_t>(heapMemEnd - heapMemStart);
}

uint32_t
MEMResizeForMBlockExpHeap(MEMHeapHandle handle,
                          virt_ptr<void> ptr,
                          uint32_t size)
{
   auto heap = virt_cast<MEMExpHeap *>(handle);
   internal::HeapLock lock { virt_addrof(heap->header) };
   size = align_up(size, 4);

   auto block = getUsedMemBlock(ptr);

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
      auto freeBlock = virt_ptr<MEMExpHeapBlock> { nullptr };

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
      auto freeMemSize = static_cast<uint32_t>(freeBlockMemEnd - freeBlockMemStart);

      // Drop the free block from the list of free regions
      removeBlock(virt_addrof(heap->freeList), freeBlock);

      // Adjust the sizing of the free area and the block
      auto newAllocSize = (size - block->blockSize);
      freeMemSize -= newAllocSize;
      block->blockSize = size;

      if (heap->header.flags & MEMHeapFlags::ZeroAllocated) {
         memset(freeBlockMemStart, 0, newAllocSize);
      } else if(heap->header.flags & MEMHeapFlags::DebugMode) {
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
MEMGetTotalFreeSizeForExpHeap(MEMHeapHandle handle)
{
   auto heap = virt_cast<MEMExpHeap *>(handle);
   auto freeSize = 0u;
   internal::HeapLock lock { virt_addrof(heap->header) };

   for (auto block = heap->freeList.head; block; block = block->next) {
      freeSize += block->blockSize;
   }

   return freeSize;
}

uint32_t
MEMGetAllocatableSizeForExpHeapEx(MEMHeapHandle handle,
                                  int32_t alignment)
{
   auto heap = virt_cast<MEMExpHeap *>(handle);
   auto largestFree = 0u;
   internal::HeapLock lock { virt_addrof(heap->header) };

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
MEMSetGroupIDForExpHeap(MEMHeapHandle handle,
                        uint16_t id)
{
   auto heap = virt_cast<MEMExpHeap *>(handle);
   internal::HeapLock lock { virt_addrof(heap->header) };
   auto originalGroupId = heap->groupId;
   heap->groupId = id;
   return originalGroupId;
}

uint16_t
MEMGetGroupIDForExpHeap(MEMHeapHandle handle)
{
   auto heap = virt_cast<MEMExpHeap *>(handle);
   internal::HeapLock lock { virt_addrof(heap->header) };
   return heap->groupId;
}

uint32_t
MEMGetSizeForMBlockExpHeap(virt_ptr<void> ptr)
{
   auto addr = virt_cast<virt_addr>(ptr);
   auto block = virt_cast<MEMExpHeapBlock *>(addr - sizeof(MEMExpHeapBlock));
   return block->blockSize;
}

uint16_t
MEMGetGroupIDForMBlockExpHeap(virt_ptr<void> ptr)
{
   auto addr = virt_cast<virt_addr>(ptr);
   auto block = virt_cast<MEMExpHeapBlock *>(addr - sizeof(MEMExpHeapBlock));
   return block->attribs.value().groupId();
}

MEMExpHeapDirection
MEMGetAllocDirForMBlockExpHeap(virt_ptr<void> ptr)
{
   auto addr = virt_cast<virt_addr>(ptr);
   auto block = virt_cast<MEMExpHeapBlock *>(addr - sizeof(MEMExpHeapBlock));
   return block->attribs.value().allocDir();
}

namespace internal
{

void
dumpExpandedHeap(virt_ptr<MEMExpHeap> heap)
{
   internal::HeapLock lock { virt_addrof(heap->header) };

   gLog->debug("MEMExpHeap(0x{:8x})", heap);
   gLog->debug("Status Address   Size       Group");

   for (auto block = heap->freeList.head; block; block = block->next) {
      auto attribs = static_cast<MEMExpHeapBlockAttribs>(block->attribs);

      gLog->debug("FREE  0x{:8x} 0x{:8x} {:d}",
                  block,
                  static_cast<uint32_t>(block->blockSize),
                  attribs.groupId());
   }

   for (auto block = heap->usedList.head; block; block = block->next) {
      auto attribs = static_cast<MEMExpHeapBlockAttribs>(block->attribs);

      gLog->debug("USED  0x{:8x} 0x{:8x} {:d}",
                  block,
                  static_cast<uint32_t>(block->blockSize),
                  attribs.groupId());
   }
}

} // namespace internal

void
Library::registerMemExpHeapSymbols()
{
   RegisterFunctionExport(MEMCreateExpHeapEx);
   RegisterFunctionExport(MEMDestroyExpHeap);
   RegisterFunctionExport(MEMAllocFromExpHeapEx);
   RegisterFunctionExport(MEMFreeToExpHeap);
   RegisterFunctionExport(MEMSetAllocModeForExpHeap);
   RegisterFunctionExport(MEMGetAllocModeForExpHeap);
   RegisterFunctionExport(MEMAdjustExpHeap);
   RegisterFunctionExport(MEMResizeForMBlockExpHeap);
   RegisterFunctionExport(MEMGetTotalFreeSizeForExpHeap);
   RegisterFunctionExport(MEMGetAllocatableSizeForExpHeapEx);
   RegisterFunctionExport(MEMSetGroupIDForExpHeap);
   RegisterFunctionExport(MEMGetGroupIDForExpHeap);
   RegisterFunctionExport(MEMGetSizeForMBlockExpHeap);
   RegisterFunctionExport(MEMGetGroupIDForMBlockExpHeap);
   RegisterFunctionExport(MEMGetAllocDirForMBlockExpHeap);
}

} // namespace cafe::coreinit

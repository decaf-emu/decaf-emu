#include "cafe_tinyheap.h"
#include <common/align.h>
#include <cstring>
#include <libcpu/be2_struct.h>

namespace cafe
{

struct TrackingBlock
{
   //! Pointer to the data heap for this block
   be2_virt_ptr<void> data;

   //! Size is negative when unallocated, positive when allocated.
   be2_val<int32_t> size;

   //! Index of next block
   be2_val<int32_t> prevBlockIdx;

   //! Index of previous block
   be2_val<int32_t> nextBlockIdx;
};
CHECK_OFFSET(TrackingBlock, 0x00, data);
CHECK_OFFSET(TrackingBlock, 0x04, size);
CHECK_OFFSET(TrackingBlock, 0x08, prevBlockIdx);
CHECK_OFFSET(TrackingBlock, 0x0C, nextBlockIdx);
CHECK_SIZE(TrackingBlock, 0x10);

static virt_ptr<TrackingBlock>
getTrackingBlocks(virt_ptr<TinyHeap> heap)
{
   return virt_cast<TrackingBlock *>(virt_cast<virt_addr>(heap) + sizeof(TinyHeap));
}

static int32_t
findBlockIdxContaining(virt_ptr<TinyHeap> heap,
                       virt_ptr<void> ptr)
{
   if (!heap || !ptr || ptr < heap->dataHeapStart || ptr >= heap->dataHeapEnd) {
      return -1;
   }

   auto trackingBlocks = getTrackingBlocks(heap);
   auto distFromStart = virt_cast<virt_addr>(ptr) - virt_cast<virt_addr>(heap->dataHeapStart);
   auto distFromEnd = virt_cast<virt_addr>(heap->dataHeapEnd) - virt_cast<virt_addr>(ptr);

   if (distFromStart < distFromEnd) {
      // Search forwards from start
      auto idx = heap->firstBlockIdx;
      while (idx >= 0) {
         auto &block = trackingBlocks[idx];
         auto blockStart = virt_cast<uint8_t *>(block.data);
         auto blockEnd = blockStart + (block.size >= 0 ? block.size : -block.size);

         if (ptr >= blockStart && ptr <= blockEnd) {
            return idx;
         }

         idx = block.nextBlockIdx;
      }
   } else {
      // Search backwards from end
      auto idx = heap->lastBlockIdx;
      while (idx >= 0) {
         auto &block = trackingBlocks[idx];
         auto blockStart = virt_cast<uint8_t *>(block.data);
         auto blockEnd = blockStart + (block.size >= 0 ? block.size : -block.size);

         if (ptr >= blockStart && ptr <= blockEnd) {
            return idx;
         }

         idx = block.prevBlockIdx;
      }
   }

   return -1;
}

TinyHeapError
TinyHeap_Setup(virt_ptr<TinyHeap> heap,
               int32_t trackingHeapSize,
               virt_ptr<void> dataHeap,
               int32_t dataHeapSize)
{
   if (trackingHeapSize < 64 || dataHeapSize <= 0) {
      return TinyHeapError::SetupFailed;
   }

   auto numTrackingBlocks = (trackingHeapSize - 0x30) / 16;
   if (numTrackingBlocks <= 0) {
      return TinyHeapError::SetupFailed;
   }

   auto trackingBlocks = getTrackingBlocks(heap);
   std::memset(trackingBlocks.getRawPointer(), 0, numTrackingBlocks * sizeof(TrackingBlock));

   for (auto i = 1; i < numTrackingBlocks; ++i) {
      trackingBlocks[i].prevBlockIdx = i - 1;
      trackingBlocks[i].nextBlockIdx = i + 1;
   }

   trackingBlocks[1].prevBlockIdx = -1;
   trackingBlocks[numTrackingBlocks - 1].nextBlockIdx = -1;

   trackingBlocks[0].data = dataHeap;
   trackingBlocks[0].size = -dataHeapSize;
   trackingBlocks[0].prevBlockIdx = -1;
   trackingBlocks[0].nextBlockIdx = -1;

   heap->dataHeapStart = dataHeap;
   heap->dataHeapEnd = virt_cast<uint8_t *>(dataHeap) + dataHeapSize;
   heap->firstBlockIdx = 0;
   heap->lastBlockIdx = 0;
   heap->nextFreeBlockIdx = 1;
   return TinyHeapError::OK;
}

static TinyHeapError
allocInBlock(virt_ptr<TinyHeap> heap,
             int32_t blockIdx,
             int32_t beforeOffset,
             int32_t holeBeforeIdx,
             int32_t holeAfterIdx,
             int32_t size)
{
   auto trackingBlocks = getTrackingBlocks(heap);
   auto &block = trackingBlocks[blockIdx];

   // Check if we need to create a hole before the allocation
   if (beforeOffset != 0) {
      auto &beforeBlock = trackingBlocks[holeBeforeIdx];
      beforeBlock.data = block.data;
      beforeBlock.size = -beforeOffset;
      beforeBlock.prevBlockIdx = block.prevBlockIdx;
      beforeBlock.nextBlockIdx = blockIdx;

      if (beforeBlock.prevBlockIdx >= 0) {
         trackingBlocks[beforeBlock.prevBlockIdx].nextBlockIdx = holeBeforeIdx;
      } else {
         heap->firstBlockIdx = holeBeforeIdx;
      }

      block.data = virt_cast<uint8_t *>(block.data) + beforeOffset;
      block.size -= -beforeOffset; // block.size is negative because unallocated
      block.prevBlockIdx = holeBeforeIdx;
   }

   // Mark the block as allocated by flipping sign of size
   block.size = -block.size;

   // Check if we need to create a hole after the allocation
   if (block.size != size) {
      auto &afterBlock = trackingBlocks[holeAfterIdx];
      afterBlock.data = virt_cast<uint8_t *>(block.data) + block.size;
      afterBlock.size = block.size - size;
      afterBlock.prevBlockIdx = blockIdx;
      afterBlock.nextBlockIdx = block.nextBlockIdx;

      if (afterBlock.nextBlockIdx >= 0) {
         trackingBlocks[afterBlock.nextBlockIdx].prevBlockIdx = holeAfterIdx;
      } else {
         heap->lastBlockIdx = holeAfterIdx;
      }

      block.size = size;
      block.nextBlockIdx = holeAfterIdx;
   }

   return TinyHeapError::OK;
}

static void
freeBlock(virt_ptr<TinyHeap> heap,
          int32_t blockIdx)
{
   auto trackingBlocks = getTrackingBlocks(heap);
   auto &block = trackingBlocks[blockIdx];

   // Mark the block as unallocated
   block.size = -block.size;

   if (block.prevBlockIdx >= 0) {
      auto prevBlockIdx = block.prevBlockIdx;
      auto &prevBlock = trackingBlocks[prevBlockIdx];
      if (prevBlock.size < 0) {
         // Merge block into prevBlock!
         prevBlock.size += block.size;
         prevBlock.nextBlockIdx = block.nextBlockIdx;

         if (prevBlock.nextBlockIdx >= 0) {
            trackingBlocks[prevBlock.nextBlockIdx].prevBlockIdx = prevBlockIdx;
         } else {
            heap->lastBlockIdx = prevBlockIdx;
         }

         // Insert block at start of free list
         block.data = nullptr;
         block.size = 0;
         block.prevBlockIdx = -1;
         block.nextBlockIdx = heap->nextFreeBlockIdx;

         if (heap->nextFreeBlockIdx >= 0) {
            trackingBlocks[heap->nextFreeBlockIdx].prevBlockIdx = blockIdx;
         }

         heap->nextFreeBlockIdx = blockIdx;

         // Set block to prevBlock so we can maybe merge with nextBlock
         block = prevBlock;
         blockIdx = prevBlockIdx;
      }
   }

   if (block.nextBlockIdx >= 0) {
      auto nextBlockIdx = block.nextBlockIdx;
      auto &nextBlock = trackingBlocks[nextBlockIdx];
      if (nextBlock.size < 0) {
         // Merge nextBlock into block!
         block.size += nextBlock.size;
         block.nextBlockIdx = nextBlockIdx;

         if (block.nextBlockIdx >= 0) {
            trackingBlocks[block.nextBlockIdx].prevBlockIdx = blockIdx;
         } else {
            heap->lastBlockIdx = blockIdx;
         }

         // Insert nextBlock at start of free list
         nextBlock.data = nullptr;
         nextBlock.size = 0;
         nextBlock.prevBlockIdx = -1;
         nextBlock.nextBlockIdx = heap->nextFreeBlockIdx;

         if (heap->nextFreeBlockIdx >= 0) {
            trackingBlocks[heap->nextFreeBlockIdx].prevBlockIdx = nextBlockIdx;
         }

         heap->nextFreeBlockIdx = nextBlockIdx;
      }
   }
}

TinyHeapError
TinyHeap_Alloc(virt_ptr<TinyHeap> heap,
               int32_t size,
               int32_t align,
               virt_ptr<void> *outPtr)
{
   if (!heap) {
      return TinyHeapError::InvalidHeap;
   }

   *outPtr = nullptr;
   if (size <= 0) {
      return TinyHeapError::OK;
   }

   auto trackingBlocks = getTrackingBlocks(heap);
   auto blockIdx = -1;

   if (align >= 0) {
      // Search forwards from first
      auto idx = heap->firstBlockIdx;

      while (idx >= 0) {
         auto &block = trackingBlocks[idx];
         if (block.size < 0) {
            auto blockStart = virt_cast<virt_addr>(block.data);
            auto blockEnd = blockStart - block.size;
            auto alignedStart = align_up(blockStart, align);

            if (alignedStart + size <= blockEnd) {
               blockIdx = idx;
               break;
            }
         }

         idx = block.nextBlockIdx;
      }
   } else {
      // Search backwards from last
      auto idx = heap->lastBlockIdx;
      align = -align;

      while (idx >= 0) {
         auto &block = trackingBlocks[idx];
         if (block.size < 0) {
            auto blockStart = virt_cast<virt_addr>(block.data);
            auto blockEnd = blockStart - block.size;
            auto alignedStart = align_up(blockStart, align);

            if (alignedStart + size <= blockEnd) {
               blockIdx = idx;
               break;
            }
         }

         idx = block.prevBlockIdx;
      }
   }

   if (blockIdx < 0) {
      // No blocks to fit this allocation in
      return TinyHeapError::AllocFailed;
   }

   auto &block = trackingBlocks[blockIdx];
   auto blockStart = virt_cast<virt_addr>(block.data);
   auto blockEnd = blockStart - block.size;
   auto alignedStart = align_up(blockStart, align);
   auto beforeOffset = static_cast<int32_t>(alignedStart - blockStart);
   auto afterOffset = blockEnd - (alignedStart + size);

   // Check if we need to insert a block before allocated block
   auto holeBeforeIdx = -1;
   if (beforeOffset != 0) {
      holeBeforeIdx = heap->nextFreeBlockIdx;
      if (holeBeforeIdx < 0) {
         // No free block to use to punch a hole
         return TinyHeapError::AllocFailed;
      }

      heap->nextFreeBlockIdx = trackingBlocks[holeBeforeIdx].nextBlockIdx;
   }

   // Check if we need to insert a block after allocated block
   auto holeAfterIdx = -1;
   if (afterOffset != 0) {
      holeAfterIdx = heap->nextFreeBlockIdx;
      if (holeAfterIdx < 0) {
         // No free block to use to punch a hole
         if (holeBeforeIdx >= 0) {
            // Restore holeBeforeIdx
            heap->nextFreeBlockIdx = holeBeforeIdx;
         }

         return TinyHeapError::AllocFailed;
      }

      heap->nextFreeBlockIdx = trackingBlocks[holeAfterIdx].nextBlockIdx;
   }

   return allocInBlock(heap, blockIdx, beforeOffset, holeBeforeIdx, holeAfterIdx, size);
}

TinyHeapError
TinyHeap_Alloc(virt_ptr<TinyHeap> heap,
               int32_t size,
               int32_t align,
               virt_ptr<virt_ptr<void>> outPtr)
{
   virt_ptr<void> tmpPtr;
   auto error = TinyHeap_Alloc(heap, size, align, &tmpPtr);
   *outPtr = tmpPtr;
   return error;
}

TinyHeapError
TinyHeap_AllocAt(virt_ptr<TinyHeap> heap,
                 virt_ptr<void> ptr,
                 int32_t size)
{
   auto blockIdx = findBlockIdxContaining(heap, ptr);
   if (blockIdx < 0) {
      // Address not in heap
      return TinyHeapError::InvalidHeap;
   }

   auto trackingBlocks = getTrackingBlocks(heap);
   auto &block = trackingBlocks[blockIdx];
   if (block.size > 0) {
      // Already allocated
      return TinyHeapError::AllocAtFailed;
   }

   auto beforeOffset = virt_cast<uint8_t *>(ptr) - virt_cast<uint8_t *>(block.data);
   auto afterOffset = (virt_cast<uint8_t *>(ptr) + -block.size) - (virt_cast<uint8_t *>(block.data) + size);
   if (afterOffset < 0) {
      // Not enough space
      return TinyHeapError::AllocAtFailed;
   }

   // Check if we need to insert a block before allocated block
   auto holeBeforeIdx = -1;
   if (beforeOffset != 0) {
      holeBeforeIdx = heap->nextFreeBlockIdx;
      if (holeBeforeIdx < 0) {
         // No free block to use to punch a hole
         return TinyHeapError::AllocAtFailed;
      }

      heap->nextFreeBlockIdx = trackingBlocks[holeBeforeIdx].nextBlockIdx;
   }

   // Check if we need to insert a block after allocated block
   auto holeAfterIdx = -1;
   if (afterOffset != 0) {
      holeAfterIdx = heap->nextFreeBlockIdx;
      if (holeAfterIdx < 0) {
         // No free block to use to punch a hole
         if (holeBeforeIdx >= 0) {
            // Restore holeBeforeIdx
            heap->nextFreeBlockIdx = holeBeforeIdx;
         }

         return TinyHeapError::AllocAtFailed;
      }

      heap->nextFreeBlockIdx = trackingBlocks[holeAfterIdx].nextBlockIdx;
   }

   return allocInBlock(heap, blockIdx, beforeOffset, holeBeforeIdx, holeAfterIdx, size);
}

void
TinyHeap_Free(virt_ptr<TinyHeap> heap,
              virt_ptr<void> ptr)
{
   auto blockIdx = findBlockIdxContaining(heap, ptr);
   if (blockIdx < 0) {
      // Address not in heap
      return;
   }

   auto trackingBlocks = getTrackingBlocks(heap);
   auto &block = trackingBlocks[blockIdx];
   if (block.data != ptr) {
      // Can only free whole blocks
      return;
   }

   if (block.size < 0) {
      // Already free
      return;
   }

   freeBlock(heap, blockIdx);
}

int32_t
TinyHeap_GetLargestFree(virt_ptr<TinyHeap> heap)
{
   if (!heap) {
      return 0;
   }

   if (heap->firstBlockIdx == -1) {
      return 0;
   }

   auto trackingBlocks = getTrackingBlocks(heap);
   auto idx = heap->firstBlockIdx;
   auto largestFree = 0;

   while (idx >= 0) {
      auto &block = trackingBlocks[idx];
      if (block.size < 0) {
         largestFree = std::max(largestFree, -block.size);
      }

      idx = block.nextBlockIdx;
   }

   return largestFree;
}

} // namespace cafe::tinyheap

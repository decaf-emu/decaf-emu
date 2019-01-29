#include "cafe_tinyheap.h"

#include <common/align.h>
#include <cstring>
#include <libcpu/be2_struct.h>

namespace cafe
{

template<typename AddressType>
struct TrackingBlockBase
{
   template<typename ValueType>
   using be2_pointer_type = be2_val<cpu::Pointer<ValueType, AddressType>>;

   //! Pointer to the data heap for this block
   be2_pointer_type<void> data;

   //! Size is negative when unallocated, positive when allocated.
   be2_val<int32_t> size;

   //! Index of next block
   be2_val<int32_t> prevBlockIdx;

   //! Index of previous block
   be2_val<int32_t> nextBlockIdx;
};
CHECK_OFFSET(TrackingBlockBase<virt_addr>, 0x00, data);
CHECK_OFFSET(TrackingBlockBase<virt_addr>, 0x04, size);
CHECK_OFFSET(TrackingBlockBase<virt_addr>, 0x08, prevBlockIdx);
CHECK_OFFSET(TrackingBlockBase<virt_addr>, 0x0C, nextBlockIdx);
CHECK_SIZE(TrackingBlockBase<virt_addr>, 0x10);

using TrackingBlockVirtual = TrackingBlockBase<virt_addr>;
using TrackingBlockPhysical = TrackingBlockBase<phys_addr>;
using TrackingBlock = TrackingBlockVirtual;

static virt_ptr<TrackingBlockVirtual>
getTrackingBlocks(virt_ptr<TinyHeapVirtual> heap)
{
   return virt_cast<TrackingBlockVirtual *>(virt_cast<virt_addr>(heap) + sizeof(TinyHeapVirtual));
}

static phys_ptr<TrackingBlockPhysical>
getTrackingBlocks(phys_ptr<TinyHeapPhysical> heap)
{
   return phys_cast<TrackingBlockPhysical *>(phys_cast<phys_addr>(heap) + sizeof(TinyHeapPhysical));
}

// TODO: Make this pointer_cast in cpu headers?
template<typename Type, typename SrcType>
static auto
pointer_cast(virt_ptr<SrcType> src)
{
   return virt_cast<Type>(src);
}

template<typename Type, typename SrcType>
static auto
pointer_cast(be2_virt_ptr<SrcType> src)
{
   return virt_cast<Type>(src);
}

template<typename Type, typename SrcType>
static auto
pointer_cast(phys_ptr<SrcType> src)
{
   return phys_cast<Type>(src);
}

template<typename Type, typename SrcType>
static auto
pointer_cast(be2_phys_ptr<SrcType> src)
{
   return phys_cast<Type>(src);
}

template<typename SrcType>
static virt_addr
pointer_to_address(virt_ptr<SrcType> src)
{
   return virt_cast<virt_addr>(src);
}

template<typename SrcType>
static virt_addr
pointer_to_address(be2_virt_ptr<SrcType> src)
{
   return virt_cast<virt_addr>(src);
}

template<typename SrcType>
static phys_addr
pointer_to_address(phys_ptr<SrcType> src)
{
   return phys_cast<phys_addr>(src);
}

template<typename SrcType>
static phys_addr
pointer_to_address(be2_phys_ptr<SrcType> src)
{
   return phys_cast<phys_addr>(src);
}

template<typename AddressType, typename SrcType>
static auto
pointer_addrof(SrcType &src)
{
   if constexpr (std::is_same<AddressType, virt_addr>::value) {
      return virt_addrof(src);
   } else {
      return phys_addrof(src);
   }
}

template<typename HeapPointer>
static void
dumpHeap(HeapPointer heap)
{
   auto idx = heap->firstBlockIdx;
   auto blocks = getTrackingBlocks(heap);

   gLog->debug("Heap at {} - {}", heap->dataHeapStart, heap->dataHeapEnd);
   while (idx != -1) {
      auto addr = pointer_to_address(blocks[idx].data);
      auto size = blocks[idx].size;
      if (size < 0) {
         size = -size;
      }

      gLog->debug("block {} - {} {}",
                  addr,
                  addr + size,
                  blocks[idx].size < 0 ? "free" : "alloc");
      idx = blocks[idx].nextBlockIdx;
   }
}

template<typename AddressType>
static int32_t
findBlockIdxContaining(cpu::Pointer<TinyHeapBase<AddressType>, AddressType> heap,
                       cpu::Pointer<void, AddressType> ptr)
{
   if (!heap || !ptr || ptr < heap->dataHeapStart || ptr >= heap->dataHeapEnd) {
      return -1;
   }

   auto trackingBlocks = getTrackingBlocks(heap);
   auto distFromStart = pointer_to_address(ptr) - pointer_to_address(heap->dataHeapStart);
   auto distFromEnd = pointer_to_address(heap->dataHeapEnd) - pointer_to_address(ptr);

   if (distFromStart < distFromEnd) {
      // Search forwards from start
      auto idx = heap->firstBlockIdx;
      while (idx >= 0) {
         auto &block = trackingBlocks[idx];
         auto blockStart = pointer_cast<uint8_t *>(block.data);
         auto blockEnd = blockStart + (block.size >= 0 ? +block.size : -block.size);

         if (ptr >= blockStart && ptr < blockEnd) {
            return idx;
         }

         idx = block.nextBlockIdx;
      }
   } else {
      // Search backwards from end
      auto idx = heap->lastBlockIdx;
      while (idx >= 0) {
         auto &block = trackingBlocks[idx];
         auto blockStart = pointer_cast<uint8_t *>(block.data);
         auto blockEnd = blockStart + (block.size >= 0 ? +block.size : -block.size);

         if (ptr >= blockStart && ptr < blockEnd) {
            return idx;
         }

         idx = block.prevBlockIdx;
      }
   }

   return -1;
}

template<typename AddressType>
TinyHeapError
TinyHeap_Setup(cpu::Pointer<TinyHeapBase<AddressType>, AddressType> heap,
               int32_t trackingHeapSize,
               cpu::Pointer<void, AddressType> dataHeap,
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
   std::memset(trackingBlocks.get(), 0, numTrackingBlocks * sizeof(TrackingBlock));

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
   heap->dataHeapEnd = pointer_cast<uint8_t *>(dataHeap) + dataHeapSize;
   heap->firstBlockIdx = 0;
   heap->lastBlockIdx = 0;
   heap->nextFreeBlockIdx = 1;
   return TinyHeapError::OK;
}

template<typename AddressType>
static TinyHeapError
allocInBlock(cpu::Pointer<TinyHeapBase<AddressType>, AddressType> heap,
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

      block.data = pointer_cast<uint8_t *>(block.data) + beforeOffset;
      block.size += beforeOffset; // += because block.size is negative at this point
      block.prevBlockIdx = holeBeforeIdx;
   }

   // Mark the block as allocated by flipping sign of size
   block.size = -block.size;

   // Check if we need to create a hole after the allocation
   if (block.size != size) {
      auto &afterBlock = trackingBlocks[holeAfterIdx];
      afterBlock.data = pointer_cast<uint8_t *>(block.data) + size;
      afterBlock.size = -(block.size - size);
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

template<typename AddressType>
static void
freeBlock(cpu::Pointer<TinyHeapBase<AddressType>, AddressType> heap,
          int32_t blockIdx)
{
   auto trackingBlocks = getTrackingBlocks(heap);
   auto block = pointer_addrof<AddressType>(trackingBlocks[blockIdx]);

   // Mark the block as unallocated
   block->size = -block->size;

   if (block->prevBlockIdx >= 0) {
      auto prevBlockIdx = block->prevBlockIdx;
      auto prevBlock = pointer_addrof<AddressType>(trackingBlocks[prevBlockIdx]);
      if (prevBlock->size < 0) {
         // Merge block into prevBlock!
         prevBlock->size += block->size;
         prevBlock->nextBlockIdx = block->nextBlockIdx;

         if (prevBlock->nextBlockIdx >= 0) {
            trackingBlocks[prevBlock->nextBlockIdx].prevBlockIdx = prevBlockIdx;
         } else {
            heap->lastBlockIdx = prevBlockIdx;
         }

         // Insert block at start of free list
         block->data = nullptr;
         block->size = 0;
         block->prevBlockIdx = -1;
         block->nextBlockIdx = heap->nextFreeBlockIdx;

         if (heap->nextFreeBlockIdx >= 0) {
            trackingBlocks[heap->nextFreeBlockIdx].prevBlockIdx = blockIdx;
         }

         heap->nextFreeBlockIdx = blockIdx;

         // Set block to prevBlock so we can maybe merge with nextBlock
         block = prevBlock;
         blockIdx = prevBlockIdx;
      }
   }

   if (block->nextBlockIdx >= 0) {
      auto nextBlockIdx = block->nextBlockIdx;
      auto nextBlock = pointer_addrof<AddressType>(trackingBlocks[nextBlockIdx]);
      if (nextBlock->size < 0) {
         // Merge nextBlock into block!
         block->size += nextBlock->size;
         block->nextBlockIdx = nextBlock->nextBlockIdx;

         if (block->nextBlockIdx >= 0) {
            trackingBlocks[block->nextBlockIdx].prevBlockIdx = blockIdx;
         } else {
            heap->lastBlockIdx = blockIdx;
         }

         // Insert nextBlock at start of free list
         nextBlock->data = nullptr;
         nextBlock->size = 0;
         nextBlock->prevBlockIdx = -1;
         nextBlock->nextBlockIdx = heap->nextFreeBlockIdx;

         if (heap->nextFreeBlockIdx >= 0) {
            trackingBlocks[heap->nextFreeBlockIdx].prevBlockIdx = nextBlockIdx;
         }

         heap->nextFreeBlockIdx = nextBlockIdx;
      }
   }
}

TinyHeapError
TinyHeap_Setup(virt_ptr<TinyHeapVirtual> heap,
               int32_t trackingHeapSize,
               virt_ptr<void> dataHeap,
               int32_t dataHeapSize)
{
   return TinyHeap_Setup<virt_addr>(heap, trackingHeapSize, dataHeap, dataHeapSize);
}

TinyHeapError
TinyHeap_Setup(phys_ptr<TinyHeapPhysical> heap,
               int32_t trackingHeapSize,
               phys_ptr<void> dataHeap,
               int32_t dataHeapSize)
{
   return TinyHeap_Setup<phys_addr>(heap, trackingHeapSize, dataHeap, dataHeapSize);
}


template<typename AddressType>
TinyHeapError
TinyHeap_Alloc(cpu::Pointer<TinyHeapBase<AddressType>, AddressType> heap,
               int32_t size,
               int32_t align,
               cpu::Pointer<void, AddressType>  *outPtr)
{
   auto fromFront = true;
   if (!heap) {
      return TinyHeapError::InvalidHeap;
   }

   *outPtr = nullptr;
   if (size <= 0) {
      return TinyHeapError::OK;
   }

   auto trackingBlocks = getTrackingBlocks(heap);
   auto blockIdx = -1;

   if (align < 0) {
      align = -align;
      fromFront = false;
   }

   if (fromFront) {
      // Search forwards from first
      auto idx = heap->firstBlockIdx;

      while (idx >= 0) {
         auto &block = trackingBlocks[idx];
         if (block.size < 0) {
            auto blockStart = pointer_to_address(block.data);
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

      while (idx >= 0) {
         auto &block = trackingBlocks[idx];
         if (block.size < 0) {
            auto blockStart = pointer_to_address(block.data);
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
   auto blockStart = pointer_to_address(block.data);
   auto blockEnd = blockStart - block.size;

   auto alignedStart = fromFront ?
      align_up(blockStart, align) :
      align_down(blockEnd - size, align);
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

   auto error = allocInBlock(heap, blockIdx, beforeOffset, holeBeforeIdx, holeAfterIdx, size);
   *outPtr = block.data;
   return error;
}

TinyHeapError
TinyHeap_Alloc(virt_ptr<TinyHeapVirtual> heap,
               int32_t size,
               int32_t align,
               virt_ptr<void> *outPtr)
{
   return TinyHeap_Alloc<virt_addr>(heap, size, align, outPtr);
}

TinyHeapError
TinyHeap_Alloc(phys_ptr<TinyHeapPhysical> heap,
               int32_t size,
               int32_t align,
               phys_ptr<void> *outPtr)
{
   return TinyHeap_Alloc<phys_addr>(heap, size, align, outPtr);
}


template<typename AddressType>
TinyHeapError
TinyHeap_AllocAt(cpu::Pointer<TinyHeapBase<AddressType>, AddressType> heap,
                 cpu::Pointer<void, AddressType> ptr,
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

   auto beforeOffset =
      static_cast<int32_t>(pointer_cast<uint8_t *>(ptr) -
                           pointer_cast<uint8_t *>(block.data));

   auto afterOffset =
      (pointer_cast<uint8_t *>(ptr) + -block.size) -
      (pointer_cast<uint8_t *>(block.data) + size);

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

   return allocInBlock(heap, blockIdx, beforeOffset, holeBeforeIdx,
                       holeAfterIdx, size);
}

TinyHeapError
TinyHeap_AllocAt(virt_ptr<TinyHeapVirtual> heap,
                 virt_ptr<void> ptr,
                 int32_t size)
{
   return TinyHeap_AllocAt<virt_addr>(heap, ptr, size);
}


TinyHeapError
TinyHeap_AllocAt(phys_ptr<TinyHeapPhysical> heap,
                 phys_ptr<void> ptr,
                 int32_t size)
{
   return TinyHeap_AllocAt<phys_addr>(heap, ptr, size);
}


template<typename AddressType>
static void
TinyHeap_Free(cpu::Pointer<TinyHeapBase<AddressType>, AddressType> heap,
              cpu::Pointer<void, AddressType> ptr)
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

void
TinyHeap_Free(virt_ptr<TinyHeapVirtual> heap,
              virt_ptr<void> ptr)
{
   TinyHeap_Free<virt_addr>(heap, ptr);
}

void
TinyHeap_Free(phys_ptr<TinyHeapPhysical> heap,
              phys_ptr<void> ptr)
{
   TinyHeap_Free<phys_addr>(heap, ptr);
}


template<typename AddressType>
static int32_t
TinyHeap_GetLargestFree(cpu::Pointer<TinyHeapBase<AddressType>, AddressType> heap)
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

int32_t
TinyHeap_GetLargestFree(virt_ptr<TinyHeapVirtual> heap)
{
   return TinyHeap_GetLargestFree<virt_addr>(heap);
}

int32_t
TinyHeap_GetLargestFree(phys_ptr<TinyHeapPhysical> heap)
{
   return TinyHeap_GetLargestFree<phys_addr>(heap);
}


template<typename AddressType>
static cpu::Pointer<void, AddressType>
TinyHeap_Enum(cpu::Pointer<TinyHeapBase<AddressType>, AddressType> heap,
              cpu::Pointer<void, AddressType> prevBlockPtr,
              cpu::Pointer<void, AddressType> *outPtr,
              uint32_t *outSize)
{
   if (!heap) {
      return nullptr;
   }

   auto blocks = getTrackingBlocks(heap);
   auto block = blocks + heap->firstBlockIdx;

   if (prevBlockPtr) {
      // Iterate through list to make sure prevBlockPtr is actually in it.
      auto prevBlock = pointer_cast<TrackingBlock *>(prevBlockPtr);
      while (block != prevBlock) {
         if (block->nextBlockIdx == -1) {
            goto error;
         }

         block = blocks + block->nextBlockIdx;
      }

      // Now we're at prevBlock, let's go to the next block
      if (block->nextBlockIdx == -1) {
         goto error;
      }

      block = blocks + block->nextBlockIdx;
   }

   if (block) {
      // Find the next allocated block
      while (block->size <= 0) {
         if (block->nextBlockIdx == -1) {
            goto error;
         }

         block = blocks + block->nextBlockIdx;
      }

      if (outPtr) {
         *outPtr = block->data;
      }

      if (outSize) {
         *outSize = static_cast<uint32_t>(block->size);
      }

      return block;
   }

error:
   if (outPtr) {
      *outPtr = nullptr;
   }

   if (outSize) {
      *outSize = 0;
   }

   return nullptr;
}

virt_ptr<void>
TinyHeap_Enum(virt_ptr<TinyHeap> heap,
              virt_ptr<void> prevBlockPtr,
              virt_ptr<void> *outPtr,
              uint32_t *outSize)
{
   return TinyHeap_Enum<virt_addr>(heap, prevBlockPtr, outPtr, outSize);
}

phys_ptr<void>
TinyHeap_Enum(phys_ptr<TinyHeapBase<phys_addr>> heap,
              phys_ptr<void> prevBlockPtr,
              phys_ptr<void> *outPtr,
              uint32_t *outSize)
{
   return TinyHeap_Enum<phys_addr>(heap, prevBlockPtr, outPtr, outSize);
}


template<typename AddressType>
static cpu::Pointer<void, AddressType>
TinyHeap_EnumFree(cpu::Pointer<TinyHeapBase<AddressType>, AddressType> heap,
                  cpu::Pointer<void, AddressType> prevBlockPtr,
                  cpu::Pointer<void, AddressType> *outPtr,
                  uint32_t *outSize)
{
   if (!heap) {
      return nullptr;
   }

   auto blocks = getTrackingBlocks(heap);
   auto block = blocks + heap->firstBlockIdx;

   if (prevBlockPtr) {
      // Iterate through list to make sure prevBlockPtr is actually in it.
      auto prevBlock = pointer_cast<TrackingBlockBase<AddressType> *>(prevBlockPtr);
      while (block != prevBlock) {
         if (block->nextBlockIdx == -1) {
            goto error;
         }

         block = blocks + block->nextBlockIdx;
      }

      // Now we're at prevBlock, let's go to the next block
      if (block->nextBlockIdx == -1) {
         goto error;
      }

      block = blocks + block->nextBlockIdx;
   }

   if (block) {
      // Find the next free block
      while (block->size >= 0) {
         if (block->nextBlockIdx == -1) {
            goto error;
         }

         block = blocks + block->nextBlockIdx;
      }

      if (outPtr) {
         *outPtr = block->data;
      }

      if (outSize) {
         *outSize = static_cast<uint32_t>(-block->size);
      }

      return block;
   }

error:
   if (outPtr) {
      *outPtr = nullptr;
   }

   if (outSize) {
      *outSize = 0;
   }

   return nullptr;
}

virt_ptr<void>
TinyHeap_EnumFree(virt_ptr<TinyHeap> heap,
                  virt_ptr<void> prevBlockPtr,
                  virt_ptr<void> *outPtr,
                  uint32_t *outSize)
{
   return TinyHeap_EnumFree<virt_addr>(heap, prevBlockPtr, outPtr, outSize);
}

phys_ptr<void>
TinyHeap_EnumFree(phys_ptr<TinyHeapBase<phys_addr>> heap,
                  phys_ptr<void> prevBlockPtr,
                  phys_ptr<void> *outPtr,
                  uint32_t *outSize)
{
   return TinyHeap_EnumFree<phys_addr>(heap, prevBlockPtr, outPtr, outSize);
}

} // namespace cafe::tinyheap

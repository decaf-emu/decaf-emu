#include "ios_kernel_heap.h"
#include "ios_kernel_process.h"

#include <array>
#include <common/decaf_assert.h>
#include <common/teenyheap.h>

namespace ios::kernel
{

constexpr auto MaxNumHeaps = 48u;
constexpr auto MaxNumProcessHeaps = 14u;
constexpr auto SharedHeapID = HeapID { 1 };
constexpr auto CrossProcessHeapID = HeapID { 0xCAFE };
constexpr auto LocalProcessHeapID = HeapID { 0xCAFE };
constexpr auto HeapAllocSizeAlign = 32u;
constexpr auto HeapAllocAlignAlign = 32u;

struct HeapData
{
   be2_array<Heap, MaxNumHeaps> heaps;
   be2_array<HeapID, MaxNumProcessHeaps> localProcessHeaps;
   be2_array<HeapID, MaxNumProcessHeaps> crossProcessHeaps;
};

static phys_ptr<HeapData>
sData;

static Error
getHeap(HeapID id,
        bool checkPermission,
        phys_ptr<Heap> *outHeap)
{
   auto error = IOS_GetCurrentProcessID();
   if (error < Error::OK) {
      return Error::Invalid;
   }

   auto pid = static_cast<ProcessID>(error);

   if (id == 0 || id == CrossProcessHeapID) {
      id = sData->crossProcessHeaps[pid];
   } else if (id == LocalProcessHeapID) {
      id = sData->localProcessHeaps[pid];
   }

   if (id >= sData->heaps.size()) {
      return Error::Invalid;
   }

   if (sData->heaps[id].pid < 0 || !sData->heaps[id].base) {
      return Error::NoExists;
   }

   if (checkPermission && sData->heaps[id].pid != pid) {
      return Error::Access;
   }

   *outHeap = phys_addrof(sData->heaps[id]);
   return Error::OK;
}

Error
IOS_CreateHeap(phys_ptr<void> ptr,
               uint32_t size)
{
   HeapID id;

   if (phys_addr { ptr }.getAddress() & 0x1F) {
      return Error::Alignment;
   }

   for (id = HeapID { 2 }; id < sData->heaps.size(); ++id) {
      if (!sData->heaps[id].base) {
         break;
      }
   }

   if (id >= sData->heaps.size()) {
      return Error::Max;
   }

   auto error = IOS_GetCurrentProcessID();
   if (error < Error::OK) {
      return error;
   }

   auto pid = static_cast<ProcessID>(error);

   // Initialise first heap block
   auto firstBlock = phys_cast<HeapBlock>(ptr);
   firstBlock->state = HeapBlockState::Free;
   firstBlock->size = static_cast<uint32_t>(size - sizeof(HeapBlock));
   firstBlock->next = nullptr;
   firstBlock->prev = nullptr;

   // Initialise heap
   auto &heap = sData->heaps[id];
   heap.id = id;
   heap.base = ptr;
   heap.size = size;
   heap.firstFreeBlock = firstBlock;
   heap.pid = pid;

   return static_cast<Error>(id);
}

Error
IOS_CreateLocalProcessHeap(phys_ptr<void> ptr,
                           uint32_t size)
{
   phys_ptr<Heap> heap;

   auto error = IOS_GetCurrentProcessID();
   if (error < Error::OK) {
      return error;
   }

   auto pid = static_cast<ProcessID>(error);
   if (pid >= sData->localProcessHeaps.size()) {
      return Error::Invalid;
   }

   if (sData->localProcessHeaps[pid] >= 0) {
      return static_cast<Error>(sData->localProcessHeaps[pid].value());
   }

   error = IOS_CreateHeap(ptr, size);
   if (error < Error::OK) {
      return error;
   }

   auto heapID = static_cast<HeapID>(error);
   error = getHeap(heapID, true, &heap);
   if (error < Error::OK) {
      return error;
   }

   heap->flags |= HeapFlags::LocalProcessHeap;
   sData->localProcessHeaps[pid] = heapID;
   return static_cast<Error>(heapID);
}

Error
IOS_CreateCrossProcessHeap(uint32_t size)
{
   phys_ptr<Heap> heap;

   auto error = IOS_GetCurrentProcessID();
   if (error < Error::OK) {
      return error;
   }

   auto pid = static_cast<ProcessID>(error);
   if (pid >= sData->crossProcessHeaps.size()) {
      return Error::Invalid;
   }

   if (sData->crossProcessHeaps[pid] >= 0) {
      return static_cast<Error>(sData->crossProcessHeaps[pid].value());
   }

   auto ptr = IOS_HeapAlloc(SharedHeapID, size);
   if (!ptr) {
      return Error::NoResource;
   }

   error = IOS_CreateHeap(ptr, size);
   if (error < Error::OK) {
      return error;
   }

   auto heapID = static_cast<HeapID>(error);
   error = getHeap(heapID, true, &heap);
   if (error < Error::OK) {
      return error;
   }

   heap->flags |= HeapFlags::CrossProcessHeap;
   sData->crossProcessHeaps[pid] = heapID;
   return static_cast<Error>(heapID);
}

Error
IOS_DestroyHeap(HeapID heapID)
{
   phys_ptr<Heap> heap;

   if (heapID == SharedHeapID) {
      return Error::Invalid;
   }

   auto error = getHeap(heapID, true, &heap);
   if (error < Error::OK) {
      return Error::Invalid;
   }

   std::memset(heap.getRawPointer(), 0, sizeof(Heap));
   heap->pid = ProcessID::Invalid;

   if (heapID == LocalProcessHeapID) {
      sData->localProcessHeaps[internal::getCurrentProcessID()] = -4;
   }

   return Error::OK;
}

phys_ptr<void>
IOS_HeapAlloc(HeapID id,
              uint32_t size)
{
   return IOS_HeapAllocAligned(id, size, 0x20);
}

phys_ptr<void>
IOS_HeapAllocAligned(HeapID heapID,
                     uint32_t size,
                     uint32_t alignment)
{
   phys_ptr<Heap> heap;

   auto error = getHeap(heapID, true, &heap);
   if (error < Error::OK || size == 0 || alignment == 0) {
      return nullptr;
   }

   auto allocBlock = phys_ptr<HeapBlock> { nullptr };
   auto allocBase = phys_ptr<uint8_t> { nullptr };

   // Both size and alignment must be aligned.
   size = align_up(size, HeapAllocSizeAlign);
   alignment = align_up(alignment, HeapAllocAlignAlign);

   for (auto block = phys_ptr<HeapBlock> { heap->firstFreeBlock }; block; block = block->next) {
      auto base = phys_cast<uint8_t>(block) + sizeof(HeapBlock);
      auto baseAddr = phys_addr { base };
      auto alignedBaseAddr = align_up(phys_addr { base }, alignment);

      decaf_check(block->state == HeapBlockState::Free);

      if (alignedBaseAddr == baseAddr) {
         // Base already aligned
         if (block->size >= size) {
            allocBlock = block;
            allocBase = base;
            break;
         }
      } else if (alignedBaseAddr - baseAddr >= sizeof(HeapBlock)) {
         // Requires alignment, and has space for a HeapBlock
         if (block->size >= size + alignment) {
            allocBlock = block;
            allocBase = alignedBaseAddr;
            size += alignment;
            break;
         }
      } else if (block->size >= size + alignment * 2) {
         // Base required double align to have space for HeapBlock
         allocBlock = block;
         allocBase = align_up(alignedBaseAddr + 1, alignment);
         size += alignment * 2;
         break;
      }
   }

   if (!allocBlock) {
      heap->errorCountAllocOutOfMemory++;
      return nullptr;
   }

   auto innerBlock = phys_cast<HeapBlock>(allocBase - sizeof(HeapBlock));
   if (innerBlock != allocBlock) {
      // Create inner block for aligned allocations if needed
      auto offset = phys_cast<uint8_t>(innerBlock) - phys_cast<uint8_t>(allocBlock);
      innerBlock->state = HeapBlockState::InnerBlock;
      innerBlock->size = static_cast<uint32_t>(size - offset - sizeof(HeapBlock));
      innerBlock->prev = allocBlock;
      innerBlock->next = nullptr;
   }

   // Check if there is enough size remaining to make it worth creating a new
   // free block.
   if (allocBlock->size - size > 0x40) {
      auto allocBlockEnd = phys_cast<uint8_t>(allocBlock) + sizeof(HeapBlock) + allocBlock->size;
      auto freeBlock = phys_cast<HeapBlock>(phys_cast<uint8_t>(allocBlock) + sizeof(HeapBlock) + size);
      freeBlock->size = static_cast<uint32_t>(allocBlockEnd - phys_cast<uint8_t>(freeBlock) - sizeof(HeapBlock));
      freeBlock->prev = allocBlock->prev;
      freeBlock->next = allocBlock->next;

      if (allocBlock->prev) {
         allocBlock->prev->next = freeBlock;
      }

      if (allocBlock->next) {
         allocBlock->next->prev = freeBlock;
      }

      allocBlock->size = size;

      if (heap->firstFreeBlock == allocBlock) {
         heap->firstFreeBlock = freeBlock;
      }
   } else if (heap->firstFreeBlock == allocBlock) {
      heap->firstFreeBlock = allocBlock->next;
   }

   allocBlock->state = HeapBlockState::Allocated;
   allocBlock->prev = nullptr;
   allocBlock->next = nullptr;

   heap->currentAllocatedSize += allocBlock->size;
   heap->largestAllocatedSize = std::max(heap->largestAllocatedSize, allocBlock->size);
   heap->totalAllocCount++;
   return allocBase;
}

static bool
heapContainsPtr(phys_ptr<Heap> heap,
                phys_ptr<void> ptr)
{
   auto start = phys_cast<uint8_t>(heap->base);
   auto end = start + heap->size;

   if (phys_cast<uint8_t>(ptr) < start + sizeof(Heap) + sizeof(HeapBlock)) {
      return false;
   }

   if (phys_cast<uint8_t>(ptr) >= end) {
      return false;
   }

   return true;
}

static void
tryMergeConsecutiveBlocks(phys_ptr<HeapBlock> first,
                          phys_ptr<HeapBlock> second)
{
   auto firstEnd = phys_cast<uint8_t>(first) + sizeof(HeapBlock) + first->size;

   if (phys_addr { firstEnd } == phys_addr { second }) {
      first->size += static_cast<uint32_t>(second->size + sizeof(HeapBlock));
      first->next = second->next;

      if (second->next) {
         second->next->prev = first;
      }
   }
}

static Error
heapFree(HeapID heapID,
         phys_ptr<void> ptr,
         bool clearMemory)
{
   phys_ptr<Heap> heap;

   if (!ptr) {
      return Error::Invalid;
   }

   auto error = getHeap(heapID, true, &heap);
   if (error < Error::OK) {
      return error;
   }

   if (!heapContainsPtr(heap, ptr)) {
      heap->errorCountFreeBlockNotInHeap++;
      return Error::Invalid;
   }

   auto freeBlock = phys_cast<HeapBlock>(phys_cast<uint8_t>(ptr) - sizeof(HeapBlock));
   if (freeBlock->state == HeapBlockState::InnerBlock) {
      freeBlock = freeBlock->prev;
   }

   if (freeBlock->state != HeapBlockState::Allocated) {
      heap->errorCountFreeUnallocatedBlock++;
      return Error::Invalid;
   }

   freeBlock->state = HeapBlockState::Free;

   if (clearMemory) {
      auto base = phys_cast<uint8_t>(freeBlock) + sizeof(HeapBlock);
      auto size = freeBlock->size;
      std::memset(base.getRawPointer(), 0, size);
   }

   heap->currentAllocatedSize -= freeBlock->size;

   auto nextBlock = heap->firstFreeBlock;
   auto prevBlock = nextBlock->prev;
   while (nextBlock) {
      if (phys_addr { freeBlock } < phys_addr { nextBlock }) {
         break;
      }

      prevBlock = nextBlock;
      nextBlock = nextBlock->next;
   }

   if (prevBlock) {
      prevBlock->next = freeBlock;
   }

   if (nextBlock) {
      nextBlock->prev = freeBlock;
   }

   freeBlock->prev = prevBlock;
   freeBlock->next = nextBlock;

   if (heap->firstFreeBlock == nextBlock) {
      heap->firstFreeBlock = freeBlock;
   }

   tryMergeConsecutiveBlocks(freeBlock, nextBlock);
   tryMergeConsecutiveBlocks(prevBlock, freeBlock);

   heap->totalFreeCount++;
   return Error::OK;
}

phys_ptr<void>
IOS_HeapRealloc(HeapID heapID,
                phys_ptr<void> ptr,
                uint32_t size)
{
   phys_ptr<Heap> heap;

   auto error = getHeap(heapID, true, &heap);
   if (error < Error::OK || !ptr) {
      return nullptr;
   }

   if (!heapContainsPtr(heap, ptr)) {
      heap->errorCountExpandInvalidBlock++;
      return nullptr;
   }

   auto block = phys_cast<HeapBlock>(phys_cast<uint8_t>(ptr) - sizeof(HeapBlock));
   auto blockSize = block->size;
   size = align_up(size, HeapAllocSizeAlign);

   if (size == 0) {
      // Realloc to size 0 just means we should free the memory.
      heapFree(heapID, ptr, false);
      return nullptr;
   } else if (block->size == size) {
      // Realloc to same size, no changes required.
      return ptr;
   }

   // Just allocate a new block and copy the data across.
   auto newPtr = IOS_HeapAllocAligned(heapID, size, HeapAllocAlignAlign);
   if (newPtr) {
      std::memcpy(newPtr.getRawPointer(),
                  ptr.getRawPointer(),
                  std::min<uint32_t>(size, block->size));
      heapFree(heapID, ptr, false);
   }

   return newPtr;
}

Error
IOS_HeapFree(HeapID id,
             phys_ptr<void> ptr)
{
   return heapFree(id, ptr, false);
}

Error
IOS_HeapFreeAndZero(HeapID id,
                    phys_ptr<void> ptr)
{
   return heapFree(id, ptr, true);
}

} // namespace ios::kernel

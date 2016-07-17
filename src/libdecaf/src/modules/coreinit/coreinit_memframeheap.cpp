#include "coreinit.h"
#include "coreinit_memheap.h"
#include "coreinit_memframeheap.h"
#include "libcpu/mem.h"
#include "libcpu/mem.h"
#include "common/align.h"
#include "virtual_ptr.h"

namespace coreinit
{

MEMFrameHeap *
MEMCreateFrmHeapEx(ppcaddr_t base,
                   uint32_t size,
                   uint32_t flags)
{
   decaf_check(base);

   // Align start and end to 4 byte boundary
   auto start = align_up(base, 4);
   auto end = align_down(start + size, 4);

   if (start >= end) {
      return nullptr;
   }

   if (end - start < sizeof(MEMFrameHeap)) {
      return nullptr;
   }

   // Setup the frame heap
   auto heap = mem::translate<MEMFrameHeap>(start);

   internal::registerHeap(&heap->header,
                          MEMHeapTag::FrameHeap,
                          start + sizeof(MEMFrameHeap),
                          end,
                          flags);

   heap->head = heap->header.dataStart;
   heap->tail = heap->header.dataEnd;
   heap->previousState = nullptr;
   return heap;
}

void *
MEMDestroyFrmHeap(MEMFrameHeap *heap)
{
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::FrameHeap);
   internal::unregisterHeap(&heap->header);
   return heap;
}

void *
MEMAllocFromFrmHeapEx(MEMFrameHeap *heap,
                      uint32_t size,
                      int alignment)
{
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::FrameHeap);

   // Yes coreinit.rpl actually does this
   if (size == 0) {
      size = 1;
   }

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Acquire(&heap->header.lock);
   }

   void *block = nullptr;

   if (alignment < 0) {
      // Allocate from bottom
      auto tail = align_down<ppcaddr_t>(heap->tail - size, -alignment);

      if (tail < heap->head) {
         // Not enough space!
         return nullptr;
      }

      heap->tail = tail;
      block = mem::translate(tail);
   } else {
      // Allocate from head
      auto addr = align_up<ppcaddr_t>(heap->head, alignment);
      auto head = addr + size;

      if (head > heap->tail) {
         // Not enough space!
         return nullptr;
      }

      heap->head = head;
      block = mem::translate(addr);
   }

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Release(&heap->header.lock);
   }

   if (heap->header.flags & MEMHeapFlags::ZeroAllocated) {
      std::memset(block, 0, size);
   } else if (heap->header.flags & MEMHeapFlags::DebugMode) {
      auto value = MEMGetFillValForHeap(MEMHeapFillType::Allocated);
      std::memset(block, value, size);
   }

   return block;
}


void
MEMFreeToFrmHeap(MEMFrameHeap *heap,
                 MEMFrameHeapFreeMode mode)
{
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::FrameHeap);

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Acquire(&heap->header.lock);
   }

   if (mode & MEMFrameHeapFreeMode::Head) {
      if (heap->header.flags & MEMHeapFlags::DebugMode) {
         auto value = MEMGetFillValForHeap(MEMHeapFillType::Freed);
         std::memset(mem::translate(heap->header.dataStart), value, heap->head - heap->header.dataStart);
      }

      heap->head = heap->header.dataStart;
      heap->previousState = nullptr;
   }

   if (mode & MEMFrameHeapFreeMode::Tail) {
      if (heap->header.flags & MEMHeapFlags::DebugMode) {
         auto value = MEMGetFillValForHeap(MEMHeapFillType::Freed);
         std::memset(mem::translate(heap->tail), value, heap->header.dataEnd - heap->tail);
      }

      heap->tail = heap->header.dataEnd;
      heap->previousState = nullptr;
   }

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Release(&heap->header.lock);
   }
}

BOOL
MEMRecordStateForFrmHeap(MEMFrameHeap *heap,
                         uint32_t tag)
{
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::FrameHeap);
   auto result = FALSE;

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Acquire(&heap->header.lock);
   }

   auto state = reinterpret_cast<MEMFrameHeapState *>(MEMAllocFromFrmHeapEx(heap, sizeof(MEMFrameHeapState), 4));

   if (state) {
      state->tag = tag;
      state->head = heap->head;
      state->tail = heap->tail;
      state->previous = heap->previousState;
      heap->previousState = state;

      result = TRUE;
   }

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Release(&heap->header.lock);
   }

   return result;
}

BOOL
MEMFreeByStateToFrmHeap(MEMFrameHeap *heap,
                        uint32_t tag)
{
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::FrameHeap);
   auto result = FALSE;

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Acquire(&heap->header.lock);
   }

   // Find the state to reset to
   auto state = heap->previousState;

   if (tag != 0) {
      while (state) {
         if (state->tag == tag) {
            break;
         }

         state = state->previous;
      }
   }

   // Reset to state
   if (state) {
      if (heap->header.flags & MEMHeapFlags::DebugMode) {
         auto value = MEMGetFillValForHeap(MEMHeapFillType::Freed);
         std::memset(mem::translate(state->head), value, heap->head - state->head);
         std::memset(mem::translate(heap->tail), value, state->tail - heap->tail);
      }

      heap->head = state->head;
      heap->tail = state->tail;
      heap->previousState = state->previous;
      result = TRUE;
   }

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Release(&heap->header.lock);
   }

   return result;
}

uint32_t
MEMAdjustFrmHeap(MEMFrameHeap *heap)
{
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::FrameHeap);
   uint32_t result = 0;

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Acquire(&heap->header.lock);
   }

   // We can only adjust the heap if we have no tail allocated memory
   if (heap->tail == heap->header.dataEnd) {
      heap->header.dataEnd = heap->head;
      heap->tail = heap->head;
      result = heap->header.dataEnd - mem::untranslate(heap);
   }

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Release(&heap->header.lock);
   }

   return result;
}

uint32_t
MEMResizeForMBlockFrmHeap(MEMFrameHeap *heap,
                          uint32_t addr,
                          uint32_t size)
{
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::FrameHeap);
   uint32_t result = 0;

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Acquire(&heap->header.lock);
   }

   decaf_check(addr > heap->head);
   decaf_check(addr < heap->tail);
   decaf_check(heap->previousState == nullptr || heap->previousState.getAddress() < addr);

   if (size == 0) {
      size = 1;
   }

   auto end = align_up(addr + size, 4);

   if (end > heap->tail) {
      // Not enough free space
      result = 0;
   } else if (end == heap->head) {
      // Same size
      result = size;
   } else if (end < heap->head) {
      // Decrease size
      if (heap->header.flags & MEMHeapFlags::DebugMode) {
         auto value = MEMGetFillValForHeap(MEMHeapFillType::Freed);
         std::memset(mem::translate(end), value, heap->head - addr);
      }

      heap->head = end;
      result = size;
   } else if (end > heap->head) {
      // Increase size
      if (heap->header.flags & MEMHeapFlags::ZeroAllocated) {
         std::memset(mem::translate(heap->head), 0, addr - heap->head);
      } else if (heap->header.flags & MEMHeapFlags::DebugMode) {
         auto value = MEMGetFillValForHeap(MEMHeapFillType::Allocated);
         std::memset(mem::translate(heap->head), value, addr - heap->head);
      }

      heap->head = end;
      result = size;
   }

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Release(&heap->header.lock);
   }

   return result;
}

uint32_t
MEMGetAllocatableSizeForFrmHeapEx(MEMFrameHeap *heap,
                                  int alignment)
{
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::FrameHeap);
   uint32_t result = 0;

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Acquire(&heap->header.lock);
   }

   auto alignedHead =  align_up<ppcaddr_t>(heap->head, alignment);

   if (alignedHead < heap->tail) {
      result = heap->tail - alignedHead;
   }

   if (heap->header.flags & MEMHeapFlags::UseLock) {
      OSUninterruptibleSpinLock_Release(&heap->header.lock);
   }

   return result;
}

void
Module::registerFrameHeapFunctions()
{
   RegisterKernelFunction(MEMCreateFrmHeapEx);
   RegisterKernelFunction(MEMDestroyFrmHeap);
   RegisterKernelFunction(MEMAllocFromFrmHeapEx);
   RegisterKernelFunction(MEMFreeToFrmHeap);
   RegisterKernelFunction(MEMRecordStateForFrmHeap);
   RegisterKernelFunction(MEMFreeByStateToFrmHeap);
   RegisterKernelFunction(MEMAdjustFrmHeap);
   RegisterKernelFunction(MEMResizeForMBlockFrmHeap);
   RegisterKernelFunction(MEMGetAllocatableSizeForFrmHeapEx);
}

} // namespace coreinit

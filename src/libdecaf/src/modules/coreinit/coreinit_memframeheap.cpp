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
MEMCreateFrmHeapEx(void *base,
                   uint32_t size,
                   uint32_t flags)
{
   decaf_check(base);

   auto baseMem = reinterpret_cast<uint8_t *>(base);

   // Align start and end to 4 byte boundary
   auto start = align_up(baseMem, 4);
   auto end = align_down(baseMem + size, 4);

   if (start >= end) {
      return nullptr;
   }

   if (end - start < sizeof(MEMFrameHeap)) {
      return nullptr;
   }

   // Setup the frame heap
   auto heap = reinterpret_cast<MEMFrameHeap *>(start);

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

   void *block = nullptr;

   {
      internal::HeapLock lock(&heap->header);

      if (alignment < 0) {
         // Allocate from bottom
         auto tail = align_down(heap->tail.get() - size, -alignment);

         if (tail < heap->head) {
            // Not enough space!
            return nullptr;
         }

         heap->tail = tail;
         block = tail;
      } else {
         // Allocate from head
         auto addr = align_up(heap->head.get(), alignment);
         auto head = addr + size;

         if (head > heap->tail) {
            // Not enough space!
            return nullptr;
         }

         heap->head = head;
         block = addr;
      }
   }

   auto heapAttribs = heap->header.attribs.value();

   if (heapAttribs.flags() & MEMHeapFlags::ZeroAllocated) {
      std::memset(block, 0, size);
   } else if (heapAttribs.flags() & MEMHeapFlags::DebugMode) {
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

   auto heapAttribs = heap->header.attribs.value();

   internal::HeapLock lock(&heap->header);

   if (mode & MEMFrameHeapFreeMode::Head) {
      if (heapAttribs.flags() & MEMHeapFlags::DebugMode) {
         auto value = MEMGetFillValForHeap(MEMHeapFillType::Freed);
         std::memset(heap->header.dataStart, value, heap->head.get() - heap->header.dataStart);
      }

      heap->head = heap->header.dataStart;
      heap->previousState = nullptr;
   }

   if (mode & MEMFrameHeapFreeMode::Tail) {
      if (heapAttribs.flags() & MEMHeapFlags::DebugMode) {
         auto value = MEMGetFillValForHeap(MEMHeapFillType::Freed);
         std::memset(heap->tail, value, heap->header.dataEnd.get() - heap->tail);
      }

      heap->tail = heap->header.dataEnd;
      heap->previousState = nullptr;
   }
}

BOOL
MEMRecordStateForFrmHeap(MEMFrameHeap *heap,
                         uint32_t tag)
{
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::FrameHeap);
   auto result = FALSE;

   auto heapAttribs = heap->header.attribs.value();

   internal::HeapLock lock(&heap->header);

   auto state = reinterpret_cast<MEMFrameHeapState *>(MEMAllocFromFrmHeapEx(heap, sizeof(MEMFrameHeapState), 4));

   if (state) {
      state->tag = tag;
      state->head = heap->head;
      state->tail = heap->tail;
      state->previous = heap->previousState;
      heap->previousState = state;

      result = TRUE;
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

   auto heapAttribs = heap->header.attribs.value();

   internal::HeapLock lock(&heap->header);

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
      if (heapAttribs.flags() & MEMHeapFlags::DebugMode) {
         auto value = MEMGetFillValForHeap(MEMHeapFillType::Freed);
         std::memset(state->head, value, heap->head.get() - state->head);
         std::memset(heap->tail, value, state->tail.get() - heap->tail);
      }

      heap->head = state->head;
      heap->tail = state->tail;
      heap->previousState = state->previous;
      result = TRUE;
   }

   return result;
}

uint32_t
MEMAdjustFrmHeap(MEMFrameHeap *heap)
{
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::FrameHeap);
   uint32_t result = 0;

   internal::HeapLock lock(&heap->header);

   // We can only adjust the heap if we have no tail allocated memory
   if (heap->tail == heap->header.dataEnd) {
      heap->header.dataEnd = heap->head;
      heap->tail = heap->head;

      auto heapMemStart = reinterpret_cast<uint8_t *>(heap);
      result = static_cast<uint32_t>(heap->header.dataEnd.get() - heapMemStart);
   }

   return result;
}

uint32_t
MEMResizeForMBlockFrmHeap(MEMFrameHeap *heap,
                          void *address,
                          uint32_t size)
{
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::FrameHeap);
   uint32_t result = 0;

   auto heapAttribs = static_cast<MEMHeapAttribs>(heap->header.attribs);

   internal::HeapLock lock(&heap->header);

   decaf_check(address > heap->head);
   decaf_check(address < heap->tail);
   decaf_check(heap->previousState == nullptr || heap->previousState < address);

   if (size == 0) {
      size = 1;
   }

   auto addrMem = reinterpret_cast<uint8_t *>(address);
   auto end = align_up(addrMem + size, 4);

   if (end > heap->tail) {
      // Not enough free space
      result = 0;
   } else if (end == heap->head) {
      // Same size
      result = size;
   } else if (end < heap->head) {
      // Decrease size
      if (heapAttribs.flags() & MEMHeapFlags::DebugMode) {
         auto value = MEMGetFillValForHeap(MEMHeapFillType::Freed);
         std::memset(end, value, heap->head.get() - addrMem);
      }

      heap->head = end;
      result = size;
   } else if (end > heap->head) {
      // Increase size
      if (heapAttribs.flags() & MEMHeapFlags::ZeroAllocated) {
         std::memset(heap->head, 0, addrMem - heap->head);
      } else if (heapAttribs.flags() & MEMHeapFlags::DebugMode) {
         auto value = MEMGetFillValForHeap(MEMHeapFillType::Allocated);
         std::memset(heap->head, value, addrMem - heap->head);
      }

      heap->head = end;
      result = size;
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

   internal::HeapLock lock(&heap->header);

   auto alignedHead =  align_up(heap->head.get(), alignment);

   if (alignedHead < heap->tail) {
      result = static_cast<uint32_t>(heap->tail.get() - alignedHead);
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

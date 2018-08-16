#include "coreinit.h"
#include "coreinit_memheap.h"
#include "coreinit_memframeheap.h"
#include "coreinit_memory.h"

namespace cafe::coreinit
{

MEMHeapHandle
MEMCreateFrmHeapEx(virt_ptr<void> base,
                   uint32_t size,
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

   if (end - start < sizeof(MEMFrameHeap)) {
      return nullptr;
   }

   // Setup the frame heap
   auto heap = virt_cast<MEMFrameHeap *>(start);

   internal::registerHeap(virt_addrof(heap->header),
                          MEMHeapTag::FrameHeap,
                          start + sizeof(MEMFrameHeap),
                          end,
                          static_cast<MEMHeapFlags>(flags));

   heap->head = heap->header.dataStart;
   heap->tail = heap->header.dataEnd;
   heap->previousState = nullptr;
   return virt_cast<MEMHeapHeader *>(heap);
}

virt_ptr<void>
MEMDestroyFrmHeap(MEMHeapHandle handle)
{
   auto heap = virt_cast<MEMFrameHeap *>(handle);
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::FrameHeap);
   internal::unregisterHeap(virt_addrof(heap->header));
   return heap;
}

virt_ptr<void>
MEMAllocFromFrmHeapEx(MEMHeapHandle handle,
                      uint32_t size,
                      int alignment)
{
   auto heap = virt_cast<MEMFrameHeap *>(handle);
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::FrameHeap);

   // Yes coreinit.rpl actually does this
   if (size == 0) {
      size = 1;
   }

   internal::HeapLock lock { virt_addrof(heap->header) };
   auto block = virt_ptr<void> { nullptr };

   if (alignment < 0) {
      // Allocate from bottom
      auto tail = align_down(heap->tail - size, -alignment);

      if (tail < heap->head) {
         // Not enough space!
         return nullptr;
      }

      heap->tail = tail;
      block = tail;
   } else {
      // Allocate from head
      auto addr = align_up(heap->head, alignment);
      auto head = addr + size;

      if (head > heap->tail) {
         // Not enough space!
         return nullptr;
      }

      heap->head = head;
      block = addr;
   }

   lock.unlock();

   if (heap->header.flags & MEMHeapFlags::ZeroAllocated) {
      memset(block, 0, size);
   } else if (heap->header.flags & MEMHeapFlags::DebugMode) {
      auto value = MEMGetFillValForHeap(MEMHeapFillType::Allocated);
      memset(block, value, size);
   }

   return block;
}


void
MEMFreeToFrmHeap(MEMHeapHandle handle,
                 MEMFrameHeapFreeMode mode)
{
   auto heap = virt_cast<MEMFrameHeap *>(handle);
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::FrameHeap);

   internal::HeapLock lock { virt_addrof(heap->header) };

   if (mode & MEMFrameHeapFreeMode::Head) {
      if (heap->header.flags & MEMHeapFlags::DebugMode) {
         auto value = MEMGetFillValForHeap(MEMHeapFillType::Freed);
         std::memset(heap->header.dataStart.getRawPointer(), value, heap->head - heap->header.dataStart);
      }

      heap->head = heap->header.dataStart;
      heap->previousState = nullptr;
   }

   if (mode & MEMFrameHeapFreeMode::Tail) {
      if (heap->header.flags & MEMHeapFlags::DebugMode) {
         auto value = MEMGetFillValForHeap(MEMHeapFillType::Freed);
         std::memset(heap->tail.getRawPointer(), value, heap->header.dataEnd - heap->tail);
      }

      heap->tail = heap->header.dataEnd;
      heap->previousState = nullptr;
   }
}

BOOL
MEMRecordStateForFrmHeap(MEMHeapHandle handle,
                         uint32_t tag)
{
   auto result = FALSE;
   auto heap = virt_cast<MEMFrameHeap *>(handle);
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::FrameHeap);

   internal::HeapLock lock { virt_addrof(heap->header) };
   auto state = virt_cast<MEMFrameHeapState *>(
      MEMAllocFromFrmHeapEx(handle, sizeof(MEMFrameHeapState), 4));

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
MEMFreeByStateToFrmHeap(MEMHeapHandle handle,
                        uint32_t tag)
{
   auto result = FALSE;
   auto heap = virt_cast<MEMFrameHeap *>(handle);
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::FrameHeap);

   internal::HeapLock lock { virt_addrof(heap->header) };

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
         std::memset(state->head.getRawPointer(), value, heap->head - state->head);
         std::memset(heap->tail.getRawPointer(), value, state->tail - heap->tail);
      }

      heap->head = state->head;
      heap->tail = state->tail;
      heap->previousState = state->previous;
      result = TRUE;
   }

   return result;
}

uint32_t
MEMAdjustFrmHeap(MEMHeapHandle handle)
{
   auto heap = virt_cast<MEMFrameHeap *>(handle);
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::FrameHeap);

   internal::HeapLock lock { virt_addrof(heap->header) };
   auto result = 0u;

   // We can only adjust the heap if we have no tail allocated memory
   if (heap->tail == heap->header.dataEnd) {
      heap->header.dataEnd = heap->head;
      heap->tail = heap->head;

      auto heapMemStart = virt_cast<uint8_t *>(heap);
      result = static_cast<uint32_t>(heap->header.dataEnd - heapMemStart);
   }

   return result;
}

uint32_t
MEMResizeForMBlockFrmHeap(MEMHeapHandle handle,
                          virt_ptr<void> address,
                          uint32_t size)
{
   auto heap = virt_cast<MEMFrameHeap *>(handle);
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::FrameHeap);

   internal::HeapLock lock { virt_addrof(heap->header) };
   auto result = 0u;

   decaf_check(address > heap->head);
   decaf_check(address < heap->tail);
   decaf_check(heap->previousState == nullptr || heap->previousState < address);

   if (size == 0) {
      size = 1;
   }

   auto addrMem = virt_cast<uint8_t *>(address);
   auto end = align_up(addrMem + size, 4);

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
         std::memset(end.getRawPointer(), value, heap->head - addrMem);
      }

      heap->head = end;
      result = size;
   } else if (end > heap->head) {
      // Increase size
      if (heap->header.flags & MEMHeapFlags::ZeroAllocated) {
         std::memset(heap->head.getRawPointer(), 0, addrMem - heap->head);
      } else if (heap->header.flags & MEMHeapFlags::DebugMode) {
         auto value = MEMGetFillValForHeap(MEMHeapFillType::Allocated);
         std::memset(heap->head.getRawPointer(), value, addrMem - heap->head);
      }

      heap->head = end;
      result = size;
   }

   return result;
}

uint32_t
MEMGetAllocatableSizeForFrmHeapEx(MEMHeapHandle handle,
                                  int alignment)
{
   auto heap = virt_cast<MEMFrameHeap *>(handle);
   decaf_check(heap);
   decaf_check(heap->header.tag == MEMHeapTag::FrameHeap);

   internal::HeapLock lock { virt_addrof(heap->header) };
   auto alignedHead = align_up(heap->head, alignment);
   auto result = 0u;

   if (alignedHead < heap->tail) {
      result = static_cast<uint32_t>(heap->tail - alignedHead);
   }

   return result;
}

void
Library::registerMemFrmHeapSymbols()
{
   RegisterFunctionExport(MEMCreateFrmHeapEx);
   RegisterFunctionExport(MEMDestroyFrmHeap);
   RegisterFunctionExport(MEMAllocFromFrmHeapEx);
   RegisterFunctionExport(MEMFreeToFrmHeap);
   RegisterFunctionExport(MEMRecordStateForFrmHeap);
   RegisterFunctionExport(MEMFreeByStateToFrmHeap);
   RegisterFunctionExport(MEMAdjustFrmHeap);
   RegisterFunctionExport(MEMResizeForMBlockFrmHeap);
   RegisterFunctionExport(MEMGetAllocatableSizeForFrmHeapEx);
}

} // namespace cafe::coreinit

#include <algorithm>
#include "coreinit.h"
#include "coreinit_memory.h"
#include "coreinit_memheap.h"
#include "coreinit_expheap.h"
#include "coreinit_frameheap.h"
#include "system.h"

p32<be_val<uint32_t>>
pMEMAllocFromDefaultHeap;

p32<be_val<uint32_t>>
pMEMAllocFromDefaultHeapEx;

p32<be_val<uint32_t>>
pMEMFreeToDefaultHeap;

static HeapHandle
gMemArenas[static_cast<size_t>(BaseHeapType::Max)];

static ExpandedHeap *
gSystemHeap = nullptr;

static MemoryList *
gForegroundMemlist = nullptr;

static MemoryList *
gMEM1Memlist = nullptr;

static MemoryList *
gMEM2Memlist = nullptr;

static MemoryList *
findListContainingHeap(CommonHeap *heap)
{
   uint32_t start, size, end;
   OSGetForegroundBucket(&start, &size);
   end = start + size;

   if (heap->dataStart >= start && heap->dataEnd < end) {
      return gForegroundMemlist;
   } else {
      OSGetMemBound(OSMemoryType::MEM1, &start, &size);
      end = start + size;

      if (heap->dataStart >= start && heap->dataEnd < end) {
         return gMEM1Memlist;
      } else {
         return gMEM2Memlist;
      }
   }
}

static MemoryList *
findListContainingBlock(void *block)
{
   uint32_t start, size, end;
   uint32_t addr = gMemory.untranslate(block);
   OSGetForegroundBucket(&start, &size);
   end = start + size;

   if (addr >= start && addr < end) {
      return gForegroundMemlist;
   } else {
      OSGetMemBound(OSMemoryType::MEM1, &start, &size);
      end = start + size;

      if (addr >= start && addr < end) {
         return gMEM1Memlist;
      } else {
         return gMEM2Memlist;
      }
   }
}

static CommonHeap *
findHeapContainingBlock(MemoryList *list, void *block)
{
   CommonHeap *heap = nullptr;
   uint32_t addr = gMemory.untranslate(block);

   while (heap = reinterpret_cast<CommonHeap*>(MEMGetNextListObject(list, heap))) {
      if (addr >= heap->dataStart && addr < heap->dataEnd) {
         auto child = findHeapContainingBlock(&heap->list, block);
         return child ? child : heap;
      }
   }

   return nullptr;
}

void
MEMiInitHeapHead(CommonHeap *heap, HeapType type, uint32_t dataStart, uint32_t dataEnd)
{
   heap->tag = type;
   MEMInitList(&heap->list, offsetof(CommonHeap, link));
   heap->dataStart = dataStart;
   heap->dataEnd = dataEnd;
   OSInitSpinLock(&heap->lock);
   heap->flags = 0;

   if (auto list = findListContainingHeap(heap)) {
      MEMAppendListObject(list, heap);
   }
}

void
MEMiFinaliseHeap(CommonHeap *heap)
{
   if (auto list = findListContainingHeap(heap)) {
      MEMRemoveListObject(list, heap);
   }
}

HeapHandle
MEMFindContainHeap(void *block)
{
   if (auto list = findListContainingBlock(block)) {
      return findHeapContainingBlock(list, block);
   }

   return nullptr;
}

BaseHeapType
MEMGetArena(HeapHandle handle)
{
   for (auto i = 0u; i < static_cast<size_t>(BaseHeapType::Max); ++i) {
      if (gMemArenas[i] == handle) {
         return static_cast<BaseHeapType>(i);
      }
   }

   return BaseHeapType::Invalid;
}

HeapHandle
MEMGetBaseHeapHandle(BaseHeapType type)
{
   if (type >= BaseHeapType::Min && type < BaseHeapType::Max) {
      return gMemArenas[static_cast<size_t>(type)];
   } else {
      return 0;
   }
}

HeapHandle
MEMSetBaseHeapHandle(BaseHeapType type, HeapHandle handle)
{
   if (type >= BaseHeapType::Min && type < BaseHeapType::Max) {
      auto previous = gMemArenas[static_cast<size_t>(type)];
      gMemArenas[static_cast<size_t>(type)] = handle;
      return previous;
   } else {
      return 0;
   }
}

static void *
sMEMAllocFromDefaultHeap(uint32_t size)
{
   auto heap = MEMGetBaseHeapHandle(BaseHeapType::MEM2);
   return MEMAllocFromExpHeap(reinterpret_cast<ExpandedHeap*>(heap), size);
}

static void *
sMEMAllocFromDefaultHeapEx(uint32_t size, int alignment)
{
   auto heap = MEMGetBaseHeapHandle(BaseHeapType::MEM2);
   return MEMAllocFromExpHeapEx(reinterpret_cast<ExpandedHeap*>(heap), size, alignment);
}

static void
sMEMFreeToDefaultHeap(p32<void> block)
{
   auto heap = MEMGetBaseHeapHandle(BaseHeapType::MEM2);
   return MEMFreeToExpHeap(reinterpret_cast<ExpandedHeap*>(heap), block);
}

void *
OSAllocFromSystem(uint32_t size, int alignment)
{
   return MEMAllocFromExpHeapEx(gSystemHeap, size, alignment);
}

void
OSFreeToSystem(void *addr)
{
   MEMFreeToExpHeap(gSystemHeap, addr);
}

void
CoreInitDefaultHeap()
{
   HeapHandle mem1, mem2, fg;
   uint32_t addr, size;

   // Create expanding heap for MEM2
   OSGetMemBound(OSMemoryType::MEM2, &addr, &size);
   addr = byte_swap(addr);
   size = byte_swap(size);
   mem2 = MEMCreateExpHeap(make_p32<ExpandedHeap>(addr), size);
   MEMSetBaseHeapHandle(BaseHeapType::MEM2, mem2);

   // Create frame heap for MEM1
   OSGetMemBound(OSMemoryType::MEM1, &addr, &size);
   addr = byte_swap(addr);
   size = byte_swap(size);
   mem1 = MEMCreateFrmHeap(make_p32<FrameHeap>(addr), size);
   MEMSetBaseHeapHandle(BaseHeapType::MEM1, mem1);

   // Create frame heap for Foreground
   OSGetForegroundBucketFreeArea(&addr, &size);
   addr = byte_swap(addr);
   size = byte_swap(size);
   fg = MEMCreateFrmHeap(make_p32<FrameHeap>(addr), size);
   MEMSetBaseHeapHandle(BaseHeapType::FG, fg);

   // Create expanding heap for System
   OSGetMemBound(OSMemoryType::System, &addr, &size);
   addr = byte_swap(addr);
   size = byte_swap(size);
   gSystemHeap = MEMCreateExpHeap(make_p32<ExpandedHeap>(addr), size);
}

void
CoreFreeDefaultHeap()
{
   // Delete all base heaps
   for (auto i = 0u; i < static_cast<size_t>(BaseHeapType::Max); ++i) {
      if (gMemArenas[i]) {
         auto heap = reinterpret_cast<CommonHeap*>(gMemArenas[i]);
         switch (heap->tag) {
         case HeapType::ExpandedHeap:
            MEMDestroyExpHeap(reinterpret_cast<ExpandedHeap*>(heap));
            break;
         case HeapType::FrameHeap:
            MEMDestroyFrmHeap(reinterpret_cast<FrameHeap*>(heap));
            break;
         case HeapType::UnitHeap:
         case HeapType::UserHeap:
         case HeapType::BlockHeap:
         default:
            assert(false);
         }

         gMemArenas[i] = nullptr;
      }
   }

   // Delete system heap
   MEMDestroyExpHeap(gSystemHeap);
   gSystemHeap = nullptr;

   // Free function pointers
   if (pMEMAllocFromDefaultHeap) {
      OSFreeToSystem(pMEMAllocFromDefaultHeap);
      pMEMAllocFromDefaultHeap = nullptr;
   }

   if (pMEMAllocFromDefaultHeapEx) {
      OSFreeToSystem(pMEMAllocFromDefaultHeap);
      pMEMAllocFromDefaultHeap = nullptr;
   }

   if (pMEMFreeToDefaultHeap) {
      OSFreeToSystem(pMEMAllocFromDefaultHeap);
      pMEMAllocFromDefaultHeap = nullptr;
   }
}

void
CoreInit::registerMembaseFunctions()
{
   memset(gMemArenas, 0, sizeof(HeapHandle) * static_cast<size_t>(BaseHeapType::Max));

   RegisterKernelFunction(MEMGetBaseHeapHandle);
   RegisterKernelFunction(MEMSetBaseHeapHandle);
   RegisterKernelFunction(MEMGetArena);
   RegisterKernelDataName("MEMAllocFromDefaultHeap", pMEMAllocFromDefaultHeap);
   RegisterKernelDataName("MEMAllocFromDefaultHeapEx", pMEMAllocFromDefaultHeapEx);
   RegisterKernelDataName("MEMFreeToDefaultHeap", pMEMFreeToDefaultHeap);
   RegisterKernelFunction(MEMiInitHeapHead);
   RegisterKernelFunction(MEMiFinaliseHeap);
   RegisterKernelFunction(MEMFindContainHeap);

   // These are default implementations for function pointers, register as exports
   // so we will have function thunks generated
   RegisterKernelFunction(sMEMAllocFromDefaultHeap);
   RegisterKernelFunction(sMEMAllocFromDefaultHeapEx);
   RegisterKernelFunction(sMEMFreeToDefaultHeap);
}

void
CoreInit::initialiseMembase()
{
   pMEMAllocFromDefaultHeap = make_p32<void>(OSAllocFromSystem(sizeof(uint32_t)));
   *pMEMAllocFromDefaultHeap = findExportAddress("sMEMAllocFromDefaultHeap");

   pMEMAllocFromDefaultHeapEx = make_p32<void>(OSAllocFromSystem(sizeof(uint32_t)));
   *pMEMAllocFromDefaultHeapEx = findExportAddress("sMEMAllocFromDefaultHeapEx");

   pMEMFreeToDefaultHeap = make_p32<void>(OSAllocFromSystem(sizeof(uint32_t)));
   *pMEMFreeToDefaultHeap = findExportAddress("sMEMFreeToDefaultHeap");

   gForegroundMemlist = OSAllocFromSystem<MemoryList>();
   MEMInitList(gForegroundMemlist, offsetof(CommonHeap, link));

   gMEM1Memlist = OSAllocFromSystem<MemoryList>();
   MEMInitList(gMEM1Memlist, offsetof(CommonHeap, link));

   gMEM2Memlist = OSAllocFromSystem<MemoryList>();
   MEMInitList(gMEM2Memlist, offsetof(CommonHeap, link));
}

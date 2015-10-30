#include <algorithm>
#include "coreinit.h"
#include "coreinit_memory.h"
#include "coreinit_memheap.h"
#include "coreinit_expheap.h"
#include "coreinit_frameheap.h"
#include "memory_translate.h"
#include "system.h"
#include "utils/teenyheap.h"
#include "utils/strutils.h"
#include "utils/virtual_ptr.h"

be_wfunc_ptr<void*, uint32_t>*
pMEMAllocFromDefaultHeap;

be_wfunc_ptr<void*, uint32_t, int>*
pMEMAllocFromDefaultHeapEx;

be_wfunc_ptr<void, void*>*
pMEMFreeToDefaultHeap;

static CommonHeap *
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
   be_val<uint32_t> start, size, end;
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
   be_val<uint32_t> start, size, end;
   uint32_t addr = memory_untranslate(block);
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
   uint32_t addr = memory_untranslate(block);

   while ((heap = reinterpret_cast<CommonHeap*>(MEMGetNextListObject(list, heap)))) {
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

void
MEMDumpHeap(CommonHeap *heap)
{
   switch (heap->tag) {
   case HeapType::ExpandedHeap:
      MEMiDumpExpHeap(reinterpret_cast<ExpandedHeap*>(heap));
      break;
   case HeapType::FrameHeap:
   case HeapType::UnitHeap:
   case HeapType::UserHeap:
   case HeapType::BlockHeap:
      gLog->info("Unimplemented MEMDumpHeap type");
   }
}

CommonHeap *
MEMFindContainHeap(void *block)
{
   if (auto list = findListContainingBlock(block)) {
      return findHeapContainingBlock(list, block);
   }

   return nullptr;
}

BaseHeapType
MEMGetArena(CommonHeap *heap)
{
   for (auto i = 0u; i < static_cast<size_t>(BaseHeapType::Max); ++i) {
      if (gMemArenas[i] == heap) {
         return static_cast<BaseHeapType>(i);
      }
   }

   return BaseHeapType::Invalid;
}

CommonHeap *
MEMGetBaseHeapHandle(BaseHeapType type)
{
   if (type >= BaseHeapType::Min && type < BaseHeapType::Max) {
      return gMemArenas[static_cast<size_t>(type)];
   } else {
      return nullptr;
   }
}

CommonHeap *
MEMSetBaseHeapHandle(BaseHeapType type, CommonHeap *heap)
{
   if (type >= BaseHeapType::Min && type < BaseHeapType::Max) {
      auto previous = gMemArenas[static_cast<size_t>(type)];
      gMemArenas[static_cast<size_t>(type)] = heap;
      return previous;
   } else {
      return nullptr;
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
sMEMFreeToDefaultHeap(uint8_t *block)
{
   auto heap = MEMGetBaseHeapHandle(BaseHeapType::MEM2);
   return MEMFreeToExpHeap(reinterpret_cast<ExpandedHeap*>(heap), block);
}

char *
OSSprintfFromSystem(const char *format, ...)
{
   va_list argptr;
   va_start(argptr, format);
   auto size = vsnprintf(nullptr, 0, format, argptr) + 1;
   auto buffer = reinterpret_cast<char *>(OSAllocFromSystem(size, 4));
   vsnprintf(buffer, size, format, argptr);
   va_end(argptr);
   return buffer;
}

void
CoreInitDefaultHeap()
{
   ExpandedHeap *mem2;
   FrameHeap *mem1, *fg;
   be_val<uint32_t> addr, size;

   // Create expanding heap for MEM2
   OSGetMemBound(OSMemoryType::MEM2, &addr, &size);
   mem2 = MEMCreateExpHeap(make_virtual_ptr<ExpandedHeap>(addr), size);
   MEMSetBaseHeapHandle(BaseHeapType::MEM2, reinterpret_cast<CommonHeap*>(mem2));

   // Create frame heap for MEM1
   OSGetMemBound(OSMemoryType::MEM1, &addr, &size);
   mem1 = MEMCreateFrmHeap(make_virtual_ptr<FrameHeap>(addr), size);
   MEMSetBaseHeapHandle(BaseHeapType::MEM1, reinterpret_cast<CommonHeap*>(mem1));

   // Create frame heap for Foreground
   OSGetForegroundBucketFreeArea(&addr, &size);
   fg = MEMCreateFrmHeap(make_virtual_ptr<FrameHeap>(addr), size);
   MEMSetBaseHeapHandle(BaseHeapType::FG, reinterpret_cast<CommonHeap*>(fg));
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
   memset(gMemArenas, 0, sizeof(CommonHeap *) * static_cast<size_t>(BaseHeapType::Max));

   RegisterKernelFunction(MEMGetBaseHeapHandle);
   RegisterKernelFunction(MEMSetBaseHeapHandle);
   RegisterKernelFunction(MEMGetArena);
   RegisterKernelDataName("MEMAllocFromDefaultHeap", pMEMAllocFromDefaultHeap);
   RegisterKernelDataName("MEMAllocFromDefaultHeapEx", pMEMAllocFromDefaultHeapEx);
   RegisterKernelDataName("MEMFreeToDefaultHeap", pMEMFreeToDefaultHeap);
   RegisterKernelFunction(MEMiInitHeapHead);
   RegisterKernelFunction(MEMiFinaliseHeap);
   RegisterKernelFunction(MEMFindContainHeap);
   RegisterKernelFunction(MEMDumpHeap);

   // These are default implementations for function pointers, register as exports
   // so we will have function thunks generated
   RegisterKernelFunction(sMEMAllocFromDefaultHeap);
   RegisterKernelFunction(sMEMAllocFromDefaultHeapEx);
   RegisterKernelFunction(sMEMFreeToDefaultHeap);
}

void
CoreInit::initialiseMembase()
{
   CoreInitDefaultHeap();

   *pMEMAllocFromDefaultHeap = findExportAddress("sMEMAllocFromDefaultHeap");
   *pMEMAllocFromDefaultHeapEx = findExportAddress("sMEMAllocFromDefaultHeapEx");
   *pMEMFreeToDefaultHeap = findExportAddress("sMEMFreeToDefaultHeap");

   gForegroundMemlist = OSAllocFromSystem<MemoryList>();
   MEMInitList(gForegroundMemlist, offsetof(CommonHeap, link));

   gMEM1Memlist = OSAllocFromSystem<MemoryList>();
   MEMInitList(gMEM1Memlist, offsetof(CommonHeap, link));

   gMEM2Memlist = OSAllocFromSystem<MemoryList>();
   MEMInitList(gMEM2Memlist, offsetof(CommonHeap, link));
}

void *
OSAllocFromSystem(uint32_t size, int alignment)
{
   auto systemHeap = gSystem.getSystemHeap();
   return systemHeap->alloc(size, alignment);
}

void
OSFreeToSystem(void *addr)
{
   auto systemHeap = gSystem.getSystemHeap();
   return systemHeap->free(addr);
}

#include <algorithm>
#include "coreinit.h"
#include "coreinit_memory.h"
#include "coreinit_memheap.h"
#include "coreinit_expheap.h"
#include "coreinit_frameheap.h"
#include "coreinit_unitheap.h"
#include "cpu/mem.h"
#include "system.h"
#include "common/teenyheap.h"
#include "common/strutils.h"
#include "common/virtual_ptr.h"

namespace coreinit
{

be_wfunc_ptr<void*, uint32_t>*
pMEMAllocFromDefaultHeap;

be_wfunc_ptr<void*, uint32_t, int>*
pMEMAllocFromDefaultHeapEx;

be_wfunc_ptr<void, void*>*
pMEMFreeToDefaultHeap;

static std::array<CommonHeap *, MEMBaseHeapType::Max>
sMemArenas;

static ExpandedHeap *
sSystemHeap = nullptr;

static MemoryList *
sForegroundMemlist = nullptr;

static MemoryList *
sMEM1Memlist = nullptr;

static MemoryList *
sMEM2Memlist = nullptr;

static MemoryList *
findListContainingHeap(CommonHeap *heap)
{
   be_val<uint32_t> start, size, end;
   OSGetForegroundBucket(&start, &size);
   end = start + size;

   if (heap->dataStart >= start && heap->dataEnd <= end) {
      return sForegroundMemlist;
   } else {
      OSGetMemBound(OSMemoryType::MEM1, &start, &size);
      end = start + size;

      if (heap->dataStart >= start && heap->dataEnd <= end) {
         return sMEM1Memlist;
      } else {
         return sMEM2Memlist;
      }
   }
}

static MemoryList *
findListContainingBlock(void *block)
{
   be_val<uint32_t> start, size, end;
   uint32_t addr = mem::untranslate(block);
   OSGetForegroundBucket(&start, &size);
   end = start + size;

   if (addr >= start && addr <= end) {
      return sForegroundMemlist;
   } else {
      OSGetMemBound(OSMemoryType::MEM1, &start, &size);
      end = start + size;

      if (addr >= start && addr <= end) {
         return sMEM1Memlist;
      } else {
         return sMEM2Memlist;
      }
   }
}

static CommonHeap *
findHeapContainingBlock(MemoryList *list, void *block)
{
   CommonHeap *heap = nullptr;
   uint32_t addr = mem::untranslate(block);

   while ((heap = reinterpret_cast<CommonHeap*>(MEMGetNextListObject(list, heap)))) {
      if (addr >= heap->dataStart && addr < heap->dataEnd) {
         auto child = findHeapContainingBlock(&heap->list, block);
         return child ? child : heap;
      }
   }

   return nullptr;
}

void
MEMiInitHeapHead(CommonHeap *heap, MEMiHeapTag tag, uint32_t dataStart, uint32_t dataEnd)
{
   heap->tag = tag;
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
   case MEMiHeapTag::ExpandedHeap:
      MEMiDumpExpHeap(reinterpret_cast<ExpandedHeap*>(heap));
      break;
   case MEMiHeapTag::UnitHeap:
      MEMiDumpUnitHeap(reinterpret_cast<UnitHeap*>(heap));
      break;
   case MEMiHeapTag::FrameHeap:
   case MEMiHeapTag::UserHeap:
   case MEMiHeapTag::BlockHeap:
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

MEMBaseHeapType
MEMGetArena(CommonHeap *heap)
{
   for (auto i = 0u; i < sMemArenas.size(); ++i) {
      if (sMemArenas[i] == heap) {
         return static_cast<MEMBaseHeapType>(i);
      }
   }

   return MEMBaseHeapType::Invalid;
}

CommonHeap *
MEMGetBaseHeapHandle(MEMBaseHeapType type)
{
   if (type < sMemArenas.size()) {
      return sMemArenas[type];
   } else {
      return nullptr;
   }
}

CommonHeap *
MEMSetBaseHeapHandle(MEMBaseHeapType type, CommonHeap *heap)
{
   if (type < sMemArenas.size()) {
      auto previous = sMemArenas[type];
      sMemArenas[type] = heap;
      return previous;
   } else {
      return nullptr;
   }
}

// TODO: Move to coreinit::internal
void
CoreInitDefaultHeap()
{
   be_val<uint32_t> addr, size;

   // Create expanding heap for MEM2
   OSGetMemBound(OSMemoryType::MEM2, &addr, &size);
   auto mem2 = MEMCreateExpHeap(make_virtual_ptr<ExpandedHeap>(addr), size);
   MEMSetBaseHeapHandle(MEMBaseHeapType::MEM2, reinterpret_cast<CommonHeap*>(mem2));

   // Create frame heap for MEM1
   OSGetMemBound(OSMemoryType::MEM1, &addr, &size);
   auto mem1 = MEMCreateFrmHeap(make_virtual_ptr<FrameHeap>(addr), size);
   MEMSetBaseHeapHandle(MEMBaseHeapType::MEM1, reinterpret_cast<CommonHeap*>(mem1));

   // Create frame heap for Foreground
   OSGetForegroundBucketFreeArea(&addr, &size);
   auto fg = MEMCreateFrmHeap(make_virtual_ptr<FrameHeap>(addr), size);
   MEMSetBaseHeapHandle(MEMBaseHeapType::FG, reinterpret_cast<CommonHeap*>(fg));
}

// TODO: Move to coreinit::internal
void
CoreFreeDefaultHeap()
{
   // Delete all base heaps
   for (auto i = 0u; i < MEMBaseHeapType::Max; ++i) {
      if (sMemArenas[i]) {
         auto heap = reinterpret_cast<CommonHeap*>(sMemArenas[i]);

         switch (heap->tag) {
         case MEMiHeapTag::ExpandedHeap:
            MEMDestroyExpHeap(reinterpret_cast<ExpandedHeap*>(heap));
            break;
         case MEMiHeapTag::FrameHeap:
            MEMDestroyFrmHeap(reinterpret_cast<FrameHeap*>(heap));
            break;
         case MEMiHeapTag::UnitHeap:
         case MEMiHeapTag::UserHeap:
         case MEMiHeapTag::BlockHeap:
         default:
            assert(false);
         }

         sMemArenas[i] = nullptr;
      }
   }

   // Delete system heap
   MEMDestroyExpHeap(sSystemHeap);
   sSystemHeap = nullptr;

   // Free function pointers
   if (pMEMAllocFromDefaultHeap) {
      coreinit::internal::sysFree(pMEMAllocFromDefaultHeap);
      pMEMAllocFromDefaultHeap = nullptr;
   }

   if (pMEMAllocFromDefaultHeapEx) {
      coreinit::internal::sysFree(pMEMAllocFromDefaultHeap);
      pMEMAllocFromDefaultHeap = nullptr;
   }

   if (pMEMFreeToDefaultHeap) {
      coreinit::internal::sysFree(pMEMAllocFromDefaultHeap);
      pMEMAllocFromDefaultHeap = nullptr;
   }
}

void
Module::registerMembaseFunctions()
{
   sMemArenas.fill(nullptr);

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
   RegisterKernelFunctionName("internal_defaultAlloc", coreinit::internal::defaultAllocFromDefaultHeap);
   RegisterKernelFunctionName("internal_defaultAllocEx", coreinit::internal::defaultAllocFromDefaultHeapEx);
   RegisterKernelFunctionName("internal_defaultFree", coreinit::internal::defaultFreeToDefaultHeap);
}

void
Module::initialiseMembase()
{
   sSystemHeap = nullptr;
   sMemArenas.fill(nullptr);

   sForegroundMemlist = coreinit::internal::sysAlloc<MemoryList>();
   MEMInitList(sForegroundMemlist, offsetof(CommonHeap, link));

   sMEM1Memlist = coreinit::internal::sysAlloc<MemoryList>();
   MEMInitList(sMEM1Memlist, offsetof(CommonHeap, link));

   sMEM2Memlist = coreinit::internal::sysAlloc<MemoryList>();
   MEMInitList(sMEM2Memlist, offsetof(CommonHeap, link));

   CoreInitDefaultHeap();

   *pMEMAllocFromDefaultHeap = findExportAddress("internal_defaultAlloc");
   *pMEMAllocFromDefaultHeapEx = findExportAddress("internal_defaultAllocEx");
   *pMEMFreeToDefaultHeap = findExportAddress("internal_defaultFree");
}

namespace internal
{

void *
sysAlloc(size_t size, int alignment)
{
   auto systemHeap = gSystem.getSystemHeap();
   return systemHeap->alloc(size, alignment);
}

void
sysFree(void *addr)
{
   auto systemHeap = gSystem.getSystemHeap();
   return systemHeap->free(addr);
}

char *
sysStrDup(const std::string &src)
{
   auto buffer = reinterpret_cast<char *>(coreinit::internal::sysAlloc(src.size() + 1, 4));
   std::memcpy(buffer, src.data(), src.size());
   buffer[src.size()] = 0;
   return buffer;
}

void *
defaultAllocFromDefaultHeap(uint32_t size)
{
   auto heap = MEMGetBaseHeapHandle(MEMBaseHeapType::MEM2);
   return MEMAllocFromExpHeap(reinterpret_cast<ExpandedHeap*>(heap), size);
}

void *
defaultAllocFromDefaultHeapEx(uint32_t size, int alignment)
{
   auto heap = MEMGetBaseHeapHandle(MEMBaseHeapType::MEM2);
   return MEMAllocFromExpHeapEx(reinterpret_cast<ExpandedHeap*>(heap), size, alignment);
}

void
defaultFreeToDefaultHeap(void *block)
{
   auto heap = MEMGetBaseHeapHandle(MEMBaseHeapType::MEM2);
   return MEMFreeToExpHeap(reinterpret_cast<ExpandedHeap*>(heap), reinterpret_cast<uint8_t *>(block));
}

} // namespace internal

} // namespace coreinit

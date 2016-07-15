#include "coreinit.h"
#include "coreinit_memory.h"
#include "coreinit_memheap.h"
#include "coreinit_expheap.h"
#include "coreinit_frameheap.h"
#include "coreinit_unitheap.h"
#include "kernel/kernel_memory.h"
#include "libcpu/mem.h"
#include "virtual_ptr.h"
#include "ppcutils/wfunc_call.h"
#include "common/decaf_assert.h"
#include "common/teenyheap.h"
#include "common/strutils.h"
#include <algorithm>
#include <array>

namespace coreinit
{

static wfunc_ptr<void*, uint32_t>
sDefaultMEMAllocFromDefaultHeap;

static wfunc_ptr<void*, uint32_t, int32_t>
sDefaultMEMAllocFromDefaultHeapEx;

static wfunc_ptr<void, void*>
sDefaultMEMFreeToDefaultHeap;

be_wfunc_ptr<void*, uint32_t> *
pMEMAllocFromDefaultHeap;

be_wfunc_ptr<void*, uint32_t, int> *
pMEMAllocFromDefaultHeapEx;

be_wfunc_ptr<void, void*> *
pMEMFreeToDefaultHeap;

static std::array<MEMHeapHeader *, MEMBaseHeapType::Max>
sMemArenas;

static MEMList *
sForegroundMemlist = nullptr;

static MEMList *
sMEM1Memlist = nullptr;

static MEMList *
sMEM2Memlist = nullptr;

static MEMList *
findListContainingHeap(MEMHeapHeader *heap)
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

static MEMList *
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

static MEMHeapHeader *
findHeapContainingBlock(MEMList *list,
                        void *block)
{
   MEMHeapHeader *heap = nullptr;
   uint32_t addr = mem::untranslate(block);

   while ((heap = reinterpret_cast<MEMHeapHeader*>(MEMGetNextListObject(list, heap)))) {
      if (addr >= heap->dataStart && addr < heap->dataEnd) {
         auto child = findHeapContainingBlock(&heap->list, block);
         return child ? child : heap;
      }
   }

   return nullptr;
}

void
MEMDumpHeap(MEMHeapHeader *heap)
{
   switch (heap->tag) {
   case MEMHeapTag::ExpandedHeap:
      internal::dumpExpandedHeap(reinterpret_cast<ExpandedHeap*>(heap));
      break;
   case MEMHeapTag::UnitHeap:
      MEMiDumpUnitHeap(reinterpret_cast<UnitHeap*>(heap));
      break;
   case MEMHeapTag::FrameHeap:
   case MEMHeapTag::UserHeap:
   case MEMHeapTag::BlockHeap:
      gLog->info("Unimplemented MEMDumpHeap type");
   }
}

MEMHeapHeader *
MEMFindContainHeap(void *block)
{
   if (auto list = findListContainingBlock(block)) {
      return findHeapContainingBlock(list, block);
   }

   return nullptr;
}

MEMBaseHeapType
MEMGetArena(MEMHeapHeader *heap)
{
   for (auto i = 0u; i < sMemArenas.size(); ++i) {
      if (sMemArenas[i] == heap) {
         return static_cast<MEMBaseHeapType>(i);
      }
   }

   return MEMBaseHeapType::Invalid;
}

MEMHeapHeader *
MEMGetBaseHeapHandle(MEMBaseHeapType type)
{
   if (type < sMemArenas.size()) {
      return sMemArenas[type];
   } else {
      return nullptr;
   }
}

MEMHeapHeader *
MEMSetBaseHeapHandle(MEMBaseHeapType type,
                     MEMHeapHeader *heap)
{
   if (type < sMemArenas.size()) {
      auto previous = sMemArenas[type];
      sMemArenas[type] = heap;
      return previous;
   } else {
      return nullptr;
   }
}

static void *
defaultAllocFromDefaultHeap(uint32_t size)
{
   auto heap = MEMGetBaseHeapHandle(MEMBaseHeapType::MEM2);
   return MEMAllocFromExpHeap(reinterpret_cast<ExpandedHeap*>(heap), size);
}

static void *
defaultAllocFromDefaultHeapEx(uint32_t size, int32_t alignment)
{
   auto heap = MEMGetBaseHeapHandle(MEMBaseHeapType::MEM2);
   return MEMAllocFromExpHeapEx(reinterpret_cast<ExpandedHeap*>(heap), size, alignment);
}

static void
defaultFreeToDefaultHeap(void *block)
{
   auto heap = MEMGetBaseHeapHandle(MEMBaseHeapType::MEM2);
   return MEMFreeToExpHeap(reinterpret_cast<ExpandedHeap*>(heap), block);
}

void
Module::registerMembaseFunctions()
{
   sMemArenas.fill(nullptr);

   RegisterKernelFunction(MEMGetBaseHeapHandle);
   RegisterKernelFunction(MEMSetBaseHeapHandle);
   RegisterKernelFunction(MEMGetArena);
   RegisterKernelFunction(MEMFindContainHeap);
   RegisterKernelFunction(MEMDumpHeap);

   RegisterKernelDataName("MEMAllocFromDefaultHeap", pMEMAllocFromDefaultHeap);
   RegisterKernelDataName("MEMAllocFromDefaultHeapEx", pMEMAllocFromDefaultHeapEx);
   RegisterKernelDataName("MEMFreeToDefaultHeap", pMEMFreeToDefaultHeap);

   RegisterInternalFunction(defaultAllocFromDefaultHeap, sDefaultMEMAllocFromDefaultHeap);
   RegisterInternalFunction(defaultAllocFromDefaultHeapEx, sDefaultMEMAllocFromDefaultHeapEx);
   RegisterInternalFunction(defaultFreeToDefaultHeap, sDefaultMEMFreeToDefaultHeap);

   RegisterInternalData(sForegroundMemlist);
   RegisterInternalData(sMEM1Memlist);
   RegisterInternalData(sMEM2Memlist);
}

void
Module::initialiseMembase()
{
   sMemArenas.fill(nullptr);

   MEMInitList(sForegroundMemlist, offsetof(MEMHeapHeader, link));
   MEMInitList(sMEM1Memlist, offsetof(MEMHeapHeader, link));
   MEMInitList(sMEM2Memlist, offsetof(MEMHeapHeader, link));

   internal::initialiseDefaultHeaps();

   // TODO: getAddress should not be neccessary here...
   *pMEMAllocFromDefaultHeap = sDefaultMEMAllocFromDefaultHeap.getAddress();
   *pMEMAllocFromDefaultHeapEx = sDefaultMEMAllocFromDefaultHeapEx.getAddress();
   *pMEMFreeToDefaultHeap = sDefaultMEMFreeToDefaultHeap.getAddress();
}

namespace internal
{

void initialiseDefaultHeaps()
{
   be_val<uint32_t> addr, size;

   // Create expanding heap for MEM2
   OSGetMemBound(OSMemoryType::MEM2, &addr, &size);
   auto mem2 = MEMCreateExpHeap(make_virtual_ptr<ExpandedHeap>(addr), size);
   MEMSetBaseHeapHandle(MEMBaseHeapType::MEM2, reinterpret_cast<MEMHeapHeader*>(mem2));

   // Create frame heap for MEM1
   OSGetMemBound(OSMemoryType::MEM1, &addr, &size);
   auto mem1 = MEMCreateFrmHeap(make_virtual_ptr<FrameHeap>(addr), size);
   MEMSetBaseHeapHandle(MEMBaseHeapType::MEM1, reinterpret_cast<MEMHeapHeader*>(mem1));

   // Create frame heap for Foreground
   OSGetForegroundBucketFreeArea(&addr, &size);
   auto fg = MEMCreateFrmHeap(make_virtual_ptr<FrameHeap>(addr), size);
   MEMSetBaseHeapHandle(MEMBaseHeapType::FG, reinterpret_cast<MEMHeapHeader*>(fg));
}


void
registerHeap(MEMHeapHeader *heap,
             MEMHeapTag tag,
             uint32_t dataStart,
             uint32_t dataEnd,
             uint32_t flags)
{
   // Setup heap header
   heap->tag = tag;
   heap->dataStart = dataStart;
   heap->dataEnd = dataEnd;
   heap->flags = flags;
   MEMInitList(&heap->list, offsetof(MEMHeapHeader, link));
   OSInitSpinLock(&heap->lock);

   // Add to heap list
   if (auto list = findListContainingHeap(heap)) {
      MEMAppendListObject(list, heap);
   }
}

void
unregisterHeap(MEMHeapHeader *heap)
{
   if (auto list = findListContainingHeap(heap)) {
      MEMRemoveListObject(list, heap);
   }
}

void *
sysAlloc(size_t size, int alignment)
{
   auto systemHeap = kernel::getSystemHeap();
   return systemHeap->alloc(size, alignment);
}

void
sysFree(void *addr)
{
   auto systemHeap = kernel::getSystemHeap();
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
allocFromDefaultHeap(uint32_t size)
{
   return (*pMEMAllocFromDefaultHeap)(size);
}

void *
allocFromDefaultHeapEx(uint32_t size, int32_t alignment)
{
   return (*pMEMAllocFromDefaultHeapEx)(size, alignment);
}

void
freeToDefaultHeap(void *block)
{
   return (*pMEMFreeToDefaultHeap)(block);
}

} // namespace internal

} // namespace coreinit

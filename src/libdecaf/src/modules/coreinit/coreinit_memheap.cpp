#include "coreinit.h"
#include "coreinit_memory.h"
#include "coreinit_memheap.h"
#include "coreinit_memexpheap.h"
#include "coreinit_memframeheap.h"
#include "coreinit_memunitheap.h"
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

static OSSpinLock *
sMemLock;

static std::array<uint32_t, MEMHeapFillType::Max>
sHeapFillVals = {
   0xC3C3C3C3,
   0xF3F3F3F3,
   0xD3D3D3D3,
};

static MEMList *
findListContainingHeap(MEMHeapHeader *heap)
{
   be_val<uint32_t> start, size, end;
   OSGetForegroundBucket(&start, &size);
   end = start + size;

   if (heap->dataStart >= mem::translate(start) && heap->dataEnd <= mem::translate(end)) {
      return sForegroundMemlist;
   } else {
      OSGetMemBound(OSMemoryType::MEM1, &start, &size);
      end = start + size;

      if (heap->dataStart >= mem::translate(start) && heap->dataEnd <= mem::translate(end)) {
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

   while ((heap = reinterpret_cast<MEMHeapHeader*>(MEMGetNextListObject(list, heap)))) {
      if (block >= heap->dataStart && block < heap->dataEnd) {
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
      internal::dumpExpandedHeap(reinterpret_cast<MEMExpHeap *>(heap));
      break;
   case MEMHeapTag::UnitHeap:
      internal::dumpUnitHeap(reinterpret_cast<MEMUnitHeap *>(heap));
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

MEMHeapHeader *
MEMCreateUserHeapHandle(MEMHeapHeader *heap,
                        uint32_t size)
{
   auto dataStart = reinterpret_cast<uint8_t *>(heap) + sizeof(MEMHeapHeader);
   auto dataEnd = dataStart + size;

   internal::registerHeap(heap,
                          coreinit::MEMHeapTag::UserHeap,
                          dataStart,
                          dataEnd,
                          0);

   return heap;
}

uint32_t
MEMGetFillValForHeap(MEMHeapFillType type)
{
   return sHeapFillVals[type];
}

void
MEMSetFillValForHeap(MEMHeapFillType type,
                     uint32_t value)
{
   sHeapFillVals[type] = value;
}

static void *
defaultAllocFromDefaultHeap(uint32_t size)
{
   auto heap = MEMGetBaseHeapHandle(MEMBaseHeapType::MEM2);
   return MEMAllocFromExpHeapEx(reinterpret_cast<MEMExpHeap*>(heap), size, 4);
}

static void *
defaultAllocFromDefaultHeapEx(uint32_t size, int32_t alignment)
{
   auto heap = MEMGetBaseHeapHandle(MEMBaseHeapType::MEM2);
   return MEMAllocFromExpHeapEx(reinterpret_cast<MEMExpHeap*>(heap), size, alignment);
}

static void
defaultFreeToDefaultHeap(void *block)
{
   auto heap = MEMGetBaseHeapHandle(MEMBaseHeapType::MEM2);
   return MEMFreeToExpHeap(reinterpret_cast<MEMExpHeap*>(heap), block);
}

void
Module::registerMembaseFunctions()
{
   sMemArenas.fill(nullptr);

   RegisterKernelFunction(MEMGetBaseHeapHandle);
   RegisterKernelFunction(MEMSetBaseHeapHandle);
   RegisterKernelFunction(MEMCreateUserHeapHandle);
   RegisterKernelFunction(MEMGetArena);
   RegisterKernelFunction(MEMFindContainHeap);
   RegisterKernelFunction(MEMDumpHeap);
   RegisterKernelFunction(MEMGetFillValForHeap);
   RegisterKernelFunction(MEMSetFillValForHeap);

   RegisterKernelDataName("MEMAllocFromDefaultHeap", pMEMAllocFromDefaultHeap);
   RegisterKernelDataName("MEMAllocFromDefaultHeapEx", pMEMAllocFromDefaultHeapEx);
   RegisterKernelDataName("MEMFreeToDefaultHeap", pMEMFreeToDefaultHeap);

   RegisterInternalFunction(defaultAllocFromDefaultHeap, sDefaultMEMAllocFromDefaultHeap);
   RegisterInternalFunction(defaultAllocFromDefaultHeapEx, sDefaultMEMAllocFromDefaultHeapEx);
   RegisterInternalFunction(defaultFreeToDefaultHeap, sDefaultMEMFreeToDefaultHeap);

   RegisterInternalData(sForegroundMemlist);
   RegisterInternalData(sMEM1Memlist);
   RegisterInternalData(sMEM2Memlist);
   RegisterInternalData(sMemLock);
}

void
Module::initialiseMembase()
{
   sMemArenas.fill(nullptr);

   MEMInitList(sForegroundMemlist, offsetof(MEMHeapHeader, link));
   MEMInitList(sMEM1Memlist, offsetof(MEMHeapHeader, link));
   MEMInitList(sMEM2Memlist, offsetof(MEMHeapHeader, link));

   OSInitSpinLock(sMemLock);

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

   auto mem2Attribs = MEMHeapAttribs::get(0)
      .useLock(true);

   // Create expanding heap for MEM2
   OSGetMemBound(OSMemoryType::MEM2, &addr, &size);
   auto mem2 = MEMCreateExpHeapEx(make_virtual_ptr<MEMExpHeap>(addr), size, mem2Attribs.value);
   MEMSetBaseHeapHandle(MEMBaseHeapType::MEM2, reinterpret_cast<MEMHeapHeader*>(mem2));

   // Create frame heap for MEM1
   OSGetMemBound(OSMemoryType::MEM1, &addr, &size);
   auto mem1 = MEMCreateFrmHeapEx(make_virtual_ptr<MEMFrameHeap>(addr), size, 0);
   MEMSetBaseHeapHandle(MEMBaseHeapType::MEM1, &mem1->header);

   // Create frame heap for Foreground
   OSGetForegroundBucketFreeArea(&addr, &size);
   auto fg = MEMCreateFrmHeapEx(make_virtual_ptr<MEMFrameHeap>(addr), size, 0);
   MEMSetBaseHeapHandle(MEMBaseHeapType::FG, &fg->header);
}


void
registerHeap(MEMHeapHeader *heap,
             MEMHeapTag tag,
             uint8_t *dataStart,
             uint8_t *dataEnd,
             uint32_t flags)
{
   // Setup heap header
   heap->tag = tag;
   heap->dataStart = dataStart;
   heap->dataEnd = dataEnd;
   heap->attribs = MEMHeapAttribs::get(flags);

   if (heap->attribs.value().debugMode()) {
      auto fillVal = MEMGetFillValForHeap(MEMHeapFillType::Unused);
      memset(dataStart, fillVal, dataEnd - dataStart);
   }

   MEMInitList(&heap->list, offsetof(MEMHeapHeader, link));

   OSInitSpinLock(&heap->lock);

   // Add to heap list
   OSUninterruptibleSpinLock_Acquire(sMemLock);

   if (auto list = findListContainingHeap(heap)) {
      MEMAppendListObject(list, heap);
   }

   OSUninterruptibleSpinLock_Release(sMemLock);
}

void
unregisterHeap(MEMHeapHeader *heap)
{
   OSUninterruptibleSpinLock_Acquire(sMemLock);

   if (auto list = findListContainingHeap(heap)) {
      MEMRemoveListObject(list, heap);
   }

   OSUninterruptibleSpinLock_Release(sMemLock);
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

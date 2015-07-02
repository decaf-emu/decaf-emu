#include <algorithm>
#include "coreinit.h"
#include "coreinit_memory.h"
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

static HeapHandle
gSystemHeap = 0;

p32<void>
OSAllocFromSystem(uint32_t size, int alignment)
{
   return MEMAllocFromExpHeapEx((ExpandedHeap*)(void*)gSystemHeap, size, alignment);
}

void
OSFreeToSystem(p32<void> addr)
{
   MEMFreeToExpHeap((ExpandedHeap*)(void*)gSystemHeap, addr);
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

static p32<void>
sMEMAllocFromDefaultHeap(uint32_t size)
{
   auto heap = MEMGetBaseHeapHandle(BaseHeapType::MEM2);
   return MEMAllocFromExpHeap((ExpandedHeap*)(void*)heap, size);
}

static p32<void>
sMEMAllocFromDefaultHeapEx(uint32_t size, int alignment)
{
   auto handle = MEMGetBaseHeapHandle(BaseHeapType::MEM2);
   return MEMAllocFromExpHeapEx((ExpandedHeap*)(void*)handle, size, alignment);
}

static void
sMEMFreeToDefaultHeap(p32<void> block)
{
   auto handle = MEMGetBaseHeapHandle(BaseHeapType::MEM2);
   return MEMFreeToExpHeap((ExpandedHeap*)(void*)handle, block);
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
   mem1 = MEMCreateFrmHeap(make_p32<void>(addr), size);
   MEMSetBaseHeapHandle(BaseHeapType::MEM1, mem1);

   // Create frame heap for Foreground
   OSGetForegroundBucketFreeArea(&addr, &size);
   addr = byte_swap(addr);
   size = byte_swap(size);
   fg = MEMCreateFrmHeap(make_p32<void>(addr), size);
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
         // TODO: Call destroy heap
         gMemArenas[i] = nullptr;
      }
   }

   // Delete system heap
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

   // These are default implementations for function pointers, register as exports
   // so we will have function thunks generated
   RegisterKernelFunction(sMEMAllocFromDefaultHeap);
   RegisterKernelFunction(sMEMAllocFromDefaultHeapEx);
   RegisterKernelFunction(sMEMFreeToDefaultHeap);
}

void
CoreInit::initialiseMembase()
{
   pMEMAllocFromDefaultHeap = OSAllocFromSystem(sizeof(uint32_t), 4);
   *pMEMAllocFromDefaultHeap = findExportAddress("sMEMAllocFromDefaultHeap");

   pMEMAllocFromDefaultHeapEx = OSAllocFromSystem(sizeof(uint32_t), 4);
   *pMEMAllocFromDefaultHeapEx = findExportAddress("sMEMAllocFromDefaultHeapEx");

   pMEMFreeToDefaultHeap = OSAllocFromSystem(sizeof(uint32_t), 4);
   *pMEMFreeToDefaultHeap = findExportAddress("sMEMFreeToDefaultHeap");
}

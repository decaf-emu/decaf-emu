#include "coreinit.h"
#include "coreinit_dynload.h"
#include "coreinit_memdefaultheap.h"
#include "coreinit_memexpheap.h"
#include "coreinit_memframeheap.h"
#include "coreinit_memheap.h"
#include "coreinit_memory.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/cafe_ppc_interface_invoke.h"

#include <libcpu/cpu.h>

namespace cafe::coreinit
{

using MEMAllocFromDefaultHeapFn = virt_func_ptr<virt_ptr<void>(uint32_t size)>;
using MEMAllocFromDefaultHeapExFn = virt_func_ptr<virt_ptr<void>(uint32_t size, int32_t align)>;
using MEMFreeToDefaultHeapFn = virt_func_ptr<void(virt_ptr<void>)>;

struct StaticDefaultHeapData
{
   be2_val<MEMHeapHandle> defaultHeapHandle;
   be2_val<uint32_t> defaultHeapAllocCount;
   be2_val<uint32_t> defaultHeapFreeCount;
};

static virt_ptr<StaticDefaultHeapData>
sDefaultHeapData = nullptr;

static virt_ptr<MEMAllocFromDefaultHeapFn> sMEMAllocFromDefaultHeap = nullptr;
static virt_ptr<MEMAllocFromDefaultHeapExFn> sMEMAllocFromDefaultHeapEx = nullptr;
static virt_ptr<MEMFreeToDefaultHeapFn> sMEMFreeToDefaultHeap = nullptr;

static MEMAllocFromDefaultHeapFn sDefaultAllocFromDefaultHeap = nullptr;
static MEMAllocFromDefaultHeapExFn sDefaultAllocFromDefaultHeapEx = nullptr;
static MEMFreeToDefaultHeapFn sDefaultFreeToDefaultHeap = nullptr;
static OSDynLoad_AllocFn sDefaultDynLoadAlloc = nullptr;
static OSDynLoad_FreeFn sDefaultDynLoadFree = nullptr;

static virt_ptr<void>
defaultAllocFromDefaultHeap(uint32_t size)
{
   return MEMAllocFromExpHeapEx(sDefaultHeapData->defaultHeapHandle,
                                size,
                                0x40u);
}

static virt_ptr<void>
defaultAllocFromDefaultHeapEx(uint32_t size,
                              int32_t alignment)
{
   return MEMAllocFromExpHeapEx(sDefaultHeapData->defaultHeapHandle,
                                size,
                                alignment);
}

static void
defaultFreeToDefaultHeap(virt_ptr<void> block)
{
   return MEMFreeToExpHeap(sDefaultHeapData->defaultHeapHandle,
                           block);
}

static OSDynLoad_Error
defaultDynLoadAlloc(int32_t size,
                    int32_t align,
                    virt_ptr<virt_ptr<void>> outPtr)
{
   if (!outPtr) {
      return OSDynLoad_Error::InvalidAllocatorPtr;
   }

   if (align >= 0 && align < 4) {
      align = 4;
   } else if (align < 0 && align > -4) {
      align = -4;
   }

   auto ptr = MEMAllocFromDefaultHeapEx(size, align);
   *outPtr = ptr;

   if (!ptr) {
      return OSDynLoad_Error::OutOfMemory;
   }

   return OSDynLoad_Error::OK;
}

static void
defaultDynLoadFree(virt_ptr<void> ptr)
{
   MEMFreeToDefaultHeap(ptr);
}

void
CoreInitDefaultHeap(virt_ptr<MEMHeapHandle> outHeapHandleMEM1,
                    virt_ptr<MEMHeapHandle> outHeapHandleFG,
                    virt_ptr<MEMHeapHandle> outHeapHandleMEM2)
{
   auto addr = StackObject<virt_addr> { };
   auto size = StackObject<uint32_t> { };

   *sMEMAllocFromDefaultHeap = sDefaultAllocFromDefaultHeap;
   *sMEMAllocFromDefaultHeapEx = sDefaultAllocFromDefaultHeapEx;
   *sMEMFreeToDefaultHeap = sDefaultFreeToDefaultHeap;

   sDefaultHeapData->defaultHeapAllocCount = 0u;
   sDefaultHeapData->defaultHeapFreeCount = 0u;

   *outHeapHandleMEM1 = nullptr;
   *outHeapHandleFG = nullptr;
   *outHeapHandleMEM2 = nullptr;

   if (OSGetForegroundBucket(nullptr, nullptr)) {
      OSGetMemBound(OSMemoryType::MEM1, addr, size);
      *outHeapHandleMEM1 = MEMCreateFrmHeapEx(virt_cast<void *>(*addr), *size, 0);

      OSGetForegroundBucketFreeArea(addr, size);
      *outHeapHandleFG = MEMCreateFrmHeapEx(virt_cast<void *>(*addr), *size, 0);
   }

   OSGetMemBound(OSMemoryType::MEM2, addr, size);
   sDefaultHeapData->defaultHeapHandle =
      MEMCreateExpHeapEx(virt_cast<void *>(*addr),
                         *size,
                         MEMHeapFlags::ThreadSafe);
   *outHeapHandleMEM2 = sDefaultHeapData->defaultHeapHandle;

   OSDynLoad_SetAllocator(sDefaultDynLoadAlloc,
                          sDefaultDynLoadFree);

   OSDynLoad_SetTLSAllocator(sDefaultDynLoadAlloc,
                             sDefaultDynLoadFree);
}

virt_ptr<void>
MEMAllocFromDefaultHeap(uint32_t size)
{
   return cafe::invoke(cpu::this_core::state(),
                       *sMEMAllocFromDefaultHeap,
                       size);

}

virt_ptr<void>
MEMAllocFromDefaultHeapEx(uint32_t size,
                          int32_t align)
{
   return cafe::invoke(cpu::this_core::state(),
                       *sMEMAllocFromDefaultHeapEx,
                       size,
                       align);

}

void
MEMFreeToDefaultHeap(virt_ptr<void> ptr)
{
   return cafe::invoke(cpu::this_core::state(),
                       *sMEMFreeToDefaultHeap,
                       ptr);

}

void
Library::registerMemDefaultHeapSymbols()
{
   RegisterFunctionExport(CoreInitDefaultHeap);
   RegisterDataExportName("MEMAllocFromDefaultHeap", sMEMAllocFromDefaultHeap);
   RegisterDataExportName("MEMAllocFromDefaultHeapEx", sMEMAllocFromDefaultHeapEx);
   RegisterDataExportName("MEMFreeToDefaultHeap", sMEMFreeToDefaultHeap);

   RegisterDataInternal(sDefaultHeapData);
   RegisterFunctionInternal(defaultAllocFromDefaultHeap, sDefaultAllocFromDefaultHeap);
   RegisterFunctionInternal(defaultAllocFromDefaultHeapEx, sDefaultAllocFromDefaultHeapEx);
   RegisterFunctionInternal(defaultFreeToDefaultHeap, sDefaultFreeToDefaultHeap);
   RegisterFunctionInternal(defaultDynLoadAlloc, sDefaultDynLoadAlloc);
   RegisterFunctionInternal(defaultDynLoadFree, sDefaultDynLoadFree);
}

} // namespace cafe::coreinit

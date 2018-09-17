#include "snduser2.h"
#include "snduser2_axfx_hooks.h"

#include "cafe/cafe_ppc_interface_invoke.h"
#include "cafe/libraries/coreinit/coreinit_memdefaultheap.h"

#include <libcpu/cpu.h>

namespace cafe::snduser2
{

struct StaticAxfxHooksData
{
   be2_val<AXFXAllocFn> allocFuncPtr;
   be2_val<AXFXFreeFn> freeFuncPtr;
};

static virt_ptr<StaticAxfxHooksData>
sAxfxHooksData = nullptr;

static AXFXAllocFn
sAxfxDefaultAlloc = nullptr;

static AXFXFreeFn
sAxfxDefaultFree = nullptr;

void
AXFXGetHooks(virt_ptr<AXFXAllocFn> allocFn,
             virt_ptr<AXFXFreeFn> freeFn)
{
   *allocFn = sAxfxHooksData->allocFuncPtr;
   *freeFn = sAxfxHooksData->freeFuncPtr;
}

void
AXFXSetHooks(AXFXAllocFn allocFn,
             AXFXFreeFn freeFn)
{
   sAxfxHooksData->allocFuncPtr = allocFn;
   sAxfxHooksData->freeFuncPtr = freeFn;
}

namespace internal
{

static virt_ptr<void>
defaultAxfxAlloc(uint32_t size)
{
   return coreinit::MEMAllocFromDefaultHeap(size);
}

static void
defaultAxfxFree(virt_ptr<void> ptr)
{
   coreinit::MEMFreeToDefaultHeap(ptr);
}

virt_ptr<void>
axfxAlloc(uint32_t size)
{
   return cafe::invoke(cpu::this_core::state(),
                       sAxfxHooksData->allocFuncPtr,
                       size);
}

void
axfxFree(virt_ptr<void> ptr)
{
   cafe::invoke(cpu::this_core::state(),
                sAxfxHooksData->freeFuncPtr,
                ptr);
}

void
initialiseAxfxHooks()
{
   sAxfxHooksData->allocFuncPtr = sAxfxDefaultAlloc;
   sAxfxHooksData->freeFuncPtr = sAxfxDefaultFree;
}

} // namespace internal

void
Library::registerAxfxHooksSymbols()
{
   RegisterFunctionExport(AXFXGetHooks);
   RegisterFunctionExport(AXFXSetHooks);

   RegisterDataInternal(sAxfxHooksData);
   RegisterFunctionInternal(internal::defaultAxfxAlloc, sAxfxDefaultAlloc);
   RegisterFunctionInternal(internal::defaultAxfxFree, sAxfxDefaultFree);
}

} // namespace cafe::snduser2

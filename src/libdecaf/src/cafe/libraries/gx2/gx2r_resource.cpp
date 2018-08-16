#include "gx2.h"
#include "gx2r_resource.h"
#include "cafe/libraries/coreinit/coreinit_memdefaultheap.h"
#include "cafe/cafe_ppc_interface_invoke.h"

#include <libcpu/be2_struct.h>
#include <libcpu/cpu.h>

namespace cafe::gx2
{

using namespace coreinit;

struct StaticGx2rResourceData
{
   be2_val<GX2RAllocFuncPtr> alloc;
   be2_val<GX2RFreeFuncPtr> free;
};

static virt_ptr<StaticGx2rResourceData> sGx2rResourceData = nullptr;
static GX2RAllocFuncPtr GX2RDefaultAlloc = nullptr;
static GX2RFreeFuncPtr GX2RDefaultFree = nullptr;

void
GX2RSetAllocator(GX2RAllocFuncPtr allocFn,
                 GX2RFreeFuncPtr freeFn)
{
   sGx2rResourceData->alloc = allocFn;
   sGx2rResourceData->free = freeFn;
}

BOOL
GX2RIsUserMemory(GX2RResourceFlags flags)
{
   return (flags & GX2RResourceFlags::Gx2rAllocated) ? FALSE : TRUE;
}

static virt_ptr<void>
defaultAlloc(GX2RResourceFlags flags,
             uint32_t size,
             uint32_t align)
{
   return MEMAllocFromDefaultHeapEx(size, align);
}

static void
defaultFree(GX2RResourceFlags flags,
            virt_ptr<void> buffer)
{
   MEMFreeToDefaultHeap(buffer);
}

namespace internal
{

GX2RResourceFlags
getOptionFlags(GX2RResourceFlags flags)
{
   // Allow flags in bits 19 to 23
   return static_cast<GX2RResourceFlags>(flags & 0xF80000);
}

virt_ptr<void>
gx2rAlloc(GX2RResourceFlags flags,
          uint32_t size,
          uint32_t align)
{
   return cafe::invoke(cpu::this_core::state(),
                       sGx2rResourceData->alloc,
                       flags,
                       size,
                       align);
}

void
gx2rFree(GX2RResourceFlags flags,
         virt_ptr<void> buffer)
{
   cafe::invoke(cpu::this_core::state(),
                sGx2rResourceData->free,
                flags,
                buffer);
}

void
initialiseGx2rAllocator()
{
   GX2RSetAllocator(GX2RDefaultAlloc, GX2RDefaultFree);
}

} // namespace internal

void
Library::registerGx2rResourceSymbols()
{
   RegisterFunctionExport(GX2RSetAllocator);
   RegisterFunctionExport(GX2RIsUserMemory);

   RegisterDataInternal(sGx2rResourceData);
   RegisterFunctionInternal(defaultAlloc, GX2RDefaultAlloc);
   RegisterFunctionInternal(defaultFree, GX2RDefaultFree);
}

} // namespace cafe::gx2

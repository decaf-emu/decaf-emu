#include "gx2.h"
#include "gx2r_resource.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "common/align.h"
#include "ppcutils/wfunc_call.h"

namespace gx2
{

static GX2RAllocFuncPtr
gGX2RMemAlloc = nullptr;

static GX2RFreeFuncPtr
gGX2RMemFree = nullptr;

void
GX2RSetAllocator(GX2RAllocFuncPtr allocFn,
                 GX2RFreeFuncPtr freeFn)
{
   gGX2RMemAlloc = allocFn;
   gGX2RMemFree = freeFn;
}

BOOL
GX2RIsUserMemory(GX2RResourceFlags flags)
{
   return (flags & GX2RResourceFlags::UserMemory) ? TRUE : FALSE;
}

namespace internal
{

void *
gx2rAlloc(GX2RResourceFlags flags, uint32_t size, uint32_t align)
{
   return gGX2RMemAlloc(flags, size, align);
}

void
gx2rFree(GX2RResourceFlags flags, void *buffer)
{
   return gGX2RMemFree(flags, buffer);
}

void *
gx2rDefaultAlloc(GX2RResourceFlags flags, uint32_t size, uint32_t align)
{
   return coreinit::internal::defaultAllocFromDefaultHeapEx(size, align);
}

void
gx2rDefaultFree(GX2RResourceFlags flags, void *buffer)
{
   coreinit::internal::defaultFreeToDefaultHeap(buffer);
}

} // namespace internal

void
Module::RegisterGX2RResourceFunctions()
{
   RegisterKernelFunction(GX2RSetAllocator);
   RegisterKernelFunction(GX2RIsUserMemory);
   RegisterKernelFunctionName("internal_gx2rDefaultAlloc", gx2::internal::gx2rDefaultAlloc);
   RegisterKernelFunctionName("internal_gx2rDefaultFree", gx2::internal::gx2rDefaultFree);
}

void
Module::initialiseResourceAllocator()
{
   gGX2RMemAlloc = findExportAddress("internal_gx2rDefaultAlloc");
   gGX2RMemFree = findExportAddress("internal_gx2rDefaultFree");
}

} // namespace gx2

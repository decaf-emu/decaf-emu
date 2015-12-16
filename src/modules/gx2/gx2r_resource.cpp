#include "gx2.h"
#include "gx2r_resource.h"
#include "utils/align.h"
#include "utils/wfunc_call.h"

static GX2RAllocFuncPtr
gGX2RMemAlloc;

static GX2RFreeFuncPtr
gGX2RMemFree;

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

namespace gx2
{

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

} // namespace internal

} // namespace gx2

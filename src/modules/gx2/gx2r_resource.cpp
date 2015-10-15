#include "gx2.h"
#include "gx2r_resource.h"

static GX2RAllocFuncPtr
pGX2RMemAlloc;

static GX2RFreeFuncPtr
pGX2RMemFree;

void
GX2RSetAllocator(GX2RAllocFuncPtr allocFn,
                 GX2RFreeFuncPtr freeFn)
{
   pGX2RMemAlloc = allocFn;
   pGX2RMemFree = freeFn;
}

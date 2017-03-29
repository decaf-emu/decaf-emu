#pragma once
#include "gx2_enum.h"
#include "ppcutils/wfunc_ptr.h"

#include <common/cbool.h>
#include <cstdint>

namespace gx2
{

using GX2RAllocFuncPtr = wfunc_ptr<void *, GX2RResourceFlags, uint32_t, uint32_t>;
using GX2RFreeFuncPtr = wfunc_ptr<void, GX2RResourceFlags, void *>;

void
GX2RSetAllocator(GX2RAllocFuncPtr allocFn,
                 GX2RFreeFuncPtr freeFn);

BOOL
GX2RIsUserMemory(GX2RResourceFlags flags);

namespace internal
{

void *
gx2rAlloc(GX2RResourceFlags flags,
          uint32_t size,
          uint32_t align);

void
gx2rFree(GX2RResourceFlags flags,
         void *buffer);

} // namespace internal

} // namespace gx2

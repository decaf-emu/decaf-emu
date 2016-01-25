#pragma once
#include "types.h"
#include "gx2_enum.h"
#include "utils/wfunc_ptr.h"

using GX2RAllocFuncPtr = wfunc_ptr<void *, GX2RResourceFlags, uint32_t, uint32_t>;
using GX2RFreeFuncPtr = wfunc_ptr<void, GX2RResourceFlags, void *>;

void
GX2RSetAllocator(GX2RAllocFuncPtr allocFn, GX2RFreeFuncPtr freeFn);

BOOL
GX2RIsUserMemory(GX2RResourceFlags flags);

namespace gx2
{

namespace internal
{

void *
gx2rAlloc(GX2RResourceFlags flags, uint32_t size, uint32_t align);

void
gx2rFree(GX2RResourceFlags flags, void *buffer);

void *
gx2rDefaultAlloc(GX2RResourceFlags flags, uint32_t size, uint32_t align);

void
gx2rDefaultFree(GX2RResourceFlags flags, void *buffer);

} // namespace internal

} // namespace gx2

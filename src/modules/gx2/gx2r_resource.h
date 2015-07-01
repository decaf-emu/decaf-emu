#pragma once
#include "systemtypes.h"

enum class GX2RResourceFlags
{
};

using GX2RAllocFuncPtr = wfunc_ptr<p32<void>, GX2RResourceFlags, uint32_t, uint32_t>;
using GX2RFreeFuncPtr = wfunc_ptr<void, GX2RResourceFlags, p32<void>>;

void
GX2RSetAllocator(GX2RAllocFuncPtr allocFn, GX2RFreeFuncPtr freeFn);

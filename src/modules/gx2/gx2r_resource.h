#pragma once
#include "systemtypes.h"

enum class GX2RResourceFlags
{
};

//using GX2RAllocFuncPtr = p32<void>(*)(GX2RResourceFlags flags, uint32_t size, uint32_t alignment);
//using GX2RFreeFuncPtr = void(*)(GX2RResourceFlags flags, void *memory);

void
GX2RSetAllocator(uint32_t allocFn, uint32_t freeFn);

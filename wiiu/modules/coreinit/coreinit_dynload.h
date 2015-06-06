#pragma once
#include "systemtypes.h"

int
OSDynLoad_SetAllocator(uint32_t allocFn, uint32_t freeFn);

int
OSDynLoad_GetAllocator(be_val<uint32_t> *outAllocFn, be_val<uint32_t> *outFreeFn);

int
OSDynLoad_MemAlloc(int size, int alignment, uint32_t *outPtr);

void
OSDynLoad_MemFree(p32<void> addr);

#pragma once
#include "systemtypes.h"

using DynLoadModuleHandle = uint32_t;

int
OSDynLoad_SetAllocator(uint32_t allocFn, uint32_t freeFn);

int
OSDynLoad_GetAllocator(be_val<uint32_t> *outAllocFn, be_val<uint32_t> *outFreeFn);

int
OSDynLoad_MemAlloc(int size, int alignment, uint32_t *outPtr);

void
OSDynLoad_MemFree(p32<void> addr);

int
OSDynLoad_Acquire(char const *name, be_val<uint32_t> *outHandle);

int
OSDynLoad_FindExport(LoadedModule *module, int isData, char const *name, be_val<uint32_t> *outAddr);

void
OSDynLoad_Release(DynLoadModuleHandle handle);

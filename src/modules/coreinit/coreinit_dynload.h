#pragma once
#include "systemtypes.h"

struct LoadedModuleHandleData;

int
OSDynLoad_SetAllocator(uint32_t allocFn, uint32_t freeFn);

int
OSDynLoad_GetAllocator(be_val<uint32_t> *outAllocFn, be_val<uint32_t> *outFreeFn);

int
OSDynLoad_MemAlloc(int size, int alignment, void **outPtr);

void
OSDynLoad_MemFree(p32<void> addr);

int
OSDynLoad_Acquire(char const *name, be_ptr<LoadedModuleHandleData> *outHandle);

int
OSDynLoad_FindExport(LoadedModuleHandleData *module, int isData, char const *name, be_ptr<void> *outAddr);

void
OSDynLoad_Release(LoadedModuleHandleData *handle);

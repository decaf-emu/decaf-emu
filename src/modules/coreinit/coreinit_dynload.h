#pragma once
#include "systemtypes.h"

struct LoadedModuleHandleData;

int
OSDynLoad_SetAllocator(ppcaddr_t allocFn, ppcaddr_t freeFn);

int
OSDynLoad_GetAllocator(be_val<ppcaddr_t> *outAllocFn, be_val<ppcaddr_t> *outFreeFn);

int
OSDynLoad_MemAlloc(int size, int alignment, void **outPtr);

void
OSDynLoad_MemFree(p32<void> addr);

int
OSDynLoad_Acquire(char const *name, be_ptr<LoadedModuleHandleData> *outHandle);

int
OSDynLoad_FindExport(LoadedModuleHandleData *module, int isData, char const *name, be_val<ppcaddr_t> *outAddr);

void
OSDynLoad_Release(LoadedModuleHandleData *handle);

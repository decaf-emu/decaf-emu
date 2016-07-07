#pragma once
#include "common/be_val.h"
#include "virtual_ptr.h"

namespace coreinit
{

namespace internal
{
   struct LoadedModuleHandleData;
}

/**
 * \defgroup coreinit_dynload Dynamic Loading
 * \ingroup coreinit
 * @{
 */

/*
Unimplemented dynamic load functions:
OSDynLoad_AcquireContainingModule
OSDynLoad_AddNofifyCallback
OSDynLoad_AddNotifyCallback
OSDynLoad_DelNotifyCallback
OSDynLoad_FindTag
OSDynLoad_GetLoaderHeapStatistics
OSDynLoad_GetModuleName
OSDynLoad_GetNumberOfRPLs
OSDynLoad_GetRPLInfo
OSDynLoad_GetTLSAllocator
OSDynLoad_IsModuleLoaded
OSDynLoad_SetTLSAllocator
*/

int
OSDynLoad_SetAllocator(ppcaddr_t allocFn,
                       ppcaddr_t freeFn);

int
OSDynLoad_GetAllocator(be_val<ppcaddr_t> *outAllocFn,
                       be_val<ppcaddr_t> *outFreeFn);

int
OSDynLoad_Acquire(char const *name, be_ptr<coreinit::internal::LoadedModuleHandleData> *outHandle);

int
OSDynLoad_FindExport(coreinit::internal::LoadedModuleHandleData *module, int isData, char const *name, be_val<ppcaddr_t> *outAddr);

void
OSDynLoad_Release(coreinit::internal::LoadedModuleHandleData *handle);

/** @} */

namespace internal
{

int
dynLoadMemAlloc(int size,
                int alignment,
                void **outPtr);

void
dynLoadMemFree(void *addr);

} // namespace internal

} // namespace coreinit

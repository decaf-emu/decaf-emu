#pragma once
#include "common/be_val.h"
#include "virtual_ptr.h"

namespace kernel
{
namespace loader
{

struct LoadedModuleHandleData;

}
}

namespace coreinit
{

using ModuleHandle = kernel::loader::LoadedModuleHandleData*;
using be_ModuleHandle = be_ptr<kernel::loader::LoadedModuleHandleData>;

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
OSDynLoad_GetNumberOfRPLs
OSDynLoad_GetRPLInfo
OSDynLoad_IsModuleLoaded
*/

int
OSDynLoad_SetAllocator(ppcaddr_t allocFn,
                       ppcaddr_t freeFn);

int
OSDynLoad_GetAllocator(be_val<ppcaddr_t> *outAllocFn,
                       be_val<ppcaddr_t> *outFreeFn);

int
OSDynLoad_SetTLSAllocator(ppcaddr_t allocFn,
                          ppcaddr_t freeFn);

int
OSDynLoad_GetTLSAllocator(be_val<ppcaddr_t> *outAllocFn,
                          be_val<ppcaddr_t> *outFreeFn);

int
OSDynLoad_Acquire(char const *name,
                  be_ModuleHandle *outHandle);

int
OSDynLoad_FindExport(ModuleHandle module,
                     int isData,
                     char const *name,
                     be_val<ppcaddr_t> *outAddr);

void
OSDynLoad_Release(ModuleHandle handle);

int
OSDynLoad_GetModuleName(ModuleHandle handle,
                        char *buffer,
                        uint32_t size);

/** @} */

namespace internal
{

int
dynLoadMemAlloc(int size,
                int alignment,
                void **outPtr);

void
dynLoadMemFree(void *addr);

int
dynLoadTLSAlloc(int size,
                int alignment,
                be_ptr<void> *outPtr);

void
dynLoadTLSFree(void *addr);

} // namespace internal

} // namespace coreinit

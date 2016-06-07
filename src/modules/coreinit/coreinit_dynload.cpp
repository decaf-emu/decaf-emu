#include <algorithm>
#include "coreinit.h"
#include "coreinit_dynload.h"
#include "coreinit_memheap.h"
#include "coreinit_expheap.h"
#include "cpu/mem.h"
#include "coreinit_internal_loader.h"
#include "common/be_val.h"
#include "common/virtual_ptr.h"
#include "ppcutils/wfunc_ptr.h"
#include "ppcutils/wfunc_call.h"

namespace coreinit
{

static wfunc_ptr<int, int, int, be_val<uint32_t>*>
sMemAlloc;

static wfunc_ptr<void, void *>
sMemFree;


/**
 * Default implementation for sMemAlloc
 */
static int
dynloadDefaultAlloc(int size, int alignment, be_val<uint32_t> *outPtr)
{
   auto heap = MEMGetBaseHeapHandle(MEMBaseHeapType::MEM2);
   auto memory = MEMAllocFromExpHeapEx(reinterpret_cast<ExpandedHeap*>(heap), size, alignment);
   *outPtr = mem::untranslate(memory);
   return 0;
}


/**
 * Default implementation for sMemFree
 */
static void
dynloadDefaultFree(void *addr)
{
   auto heap = MEMGetBaseHeapHandle(MEMBaseHeapType::MEM2);
   MEMFreeToExpHeap(reinterpret_cast<ExpandedHeap*>(heap), addr);
}


/**
 * Set the allocator to be used for allocating data sections for dynamically loaded libraries.
 */
int
OSDynLoad_SetAllocator(ppcaddr_t allocFn, ppcaddr_t freeFn)
{
   if (!allocFn || !freeFn) {
      return 0xBAD10017;
   }

   sMemAlloc = allocFn;
   sMemFree = freeFn;
   return 0;
}


/**
 * Return the allocators set by OSDynLoad_SetAllocator.
 */
int
OSDynLoad_GetAllocator(be_val<ppcaddr_t> *outAllocFn, be_val<ppcaddr_t> *outFreeFn)
{
   *outAllocFn = static_cast<ppcaddr_t>(sMemAlloc);
   *outFreeFn = static_cast<ppcaddr_t>(sMemFree);
   return 0;
}


/**
 * Load and return the handle of a dynamic library.
 *
 * If the library is already loaded then increase ref count.
 */
int
OSDynLoad_Acquire(char const *name, be_ptr<coreinit::internal::LoadedModuleHandleData> *outHandle)
{
   auto module = coreinit::internal::loadRPL(name);

   if (!module) {
      gLog->debug("OSDynLoad_Acquire {} failed", name);
      *outHandle = nullptr;
      return 0xBAD10001;
   }

   *outHandle = module->handle;
   return 0;
}


/**
 * Find the export from a library handle.
 */
int
OSDynLoad_FindExport(coreinit::internal::LoadedModuleHandleData *handle, int isData, char const *name, be_val<ppcaddr_t> *outAddr)
{
   auto module = handle->ptr;
   auto exportPtr = module->findExport(name);

   if (!exportPtr) {
      gLog->debug("OSDynLoad_FindExport export {} not found", name);
      *outAddr = 0;
      return 0xBAD10001;
   }

   *outAddr = exportPtr;
   return 0;
}


/**
 * Release handle of loaded library.
 *
 * Decreases the ref count of the library pointed to by handle.
 * The library is unloaded if ref count hits 0.
 */
void
OSDynLoad_Release(coreinit::internal::LoadedModuleHandleData *handle)
{
   // TODO: Unload library when ref count hits 0
}

void
Module::initialiseDynLoad()
{
}

void
Module::registerDynLoadFunctions()
{
   RegisterKernelFunction(OSDynLoad_Acquire);
   RegisterKernelFunction(OSDynLoad_FindExport);
   RegisterKernelFunction(OSDynLoad_Release);
   RegisterKernelFunction(OSDynLoad_SetAllocator);
   RegisterKernelFunction(OSDynLoad_GetAllocator);

   RegisterInternalFunction(dynloadDefaultAlloc, sMemAlloc);
   RegisterInternalFunction(dynloadDefaultFree, sMemFree);
}

namespace internal
{

/**
 * Wrapper func to call function pointer set by OSDynLoad_SetAllocator
 */
int
dynLoadMemAlloc(int size, int alignment, void **outPtr)
{
   auto value = coreinit::internal::sysAlloc<be_val<ppcaddr_t>>();
   auto result = sMemAlloc(size, alignment, value);
   *outPtr = mem::translate(*value);
   coreinit::internal::sysFree(value);
   return result;
}


/**
 * Wrapper func to call function pointer set by OSDynLoad_SetAllocator
 */
void
dynLoadMemFree(void *addr)
{
   sMemFree(addr);
}

} // namespace internal

} // namespace coreinit

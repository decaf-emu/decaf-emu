#include <algorithm>
#include "common/be_val.h"
#include "coreinit.h"
#include "coreinit_dynload.h"
#include "coreinit_memexpheap.h"
#include "coreinit_memheap.h"
#include "kernel/kernel_loader.h"
#include "libcpu/mem.h"
#include "ppcutils/wfunc_ptr.h"
#include "ppcutils/wfunc_call.h"
#include "virtual_ptr.h"

namespace coreinit
{

static wfunc_ptr<int, int, int, be_ptr<void> *>
sMemAlloc;

static wfunc_ptr<void, void *>
sMemFree;

static wfunc_ptr<int, int, int, be_ptr<void> *>
sTLSAlloc;

static wfunc_ptr<void, void *>
sTLSFree;


/**
 * Default implementation for sMemAlloc
 */
static int
dynloadDefaultAlloc(int size,
                    int alignment,
                    be_ptr<void> *outPtr)
{
   *outPtr = internal::allocFromDefaultHeapEx(size, alignment);
   return 0;
}


/**
 * Default implementation for sMemFree
 */
static void
dynloadDefaultFree(void *addr)
{
   internal::freeToDefaultHeap(addr);
}


/**
 * Default implementation for sTLSAlloc
 */
static int
tlsDefaultAlloc(int size,
                int alignment,
                be_ptr<void> *outPtr)
{
   *outPtr = internal::allocFromDefaultHeapEx(size, alignment);
   return 0;
}


/**
 * Default implementation for sTLSFree
 */
static void
tlsDefaultFree(void *addr)
{
   internal::freeToDefaultHeap(addr);
}


/**
 * Set the allocator to be used for allocating data sections for dynamically loaded libraries.
 */
int
OSDynLoad_SetAllocator(ppcaddr_t allocFn,
                       ppcaddr_t freeFn)
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
OSDynLoad_GetAllocator(be_val<ppcaddr_t> *outAllocFn,
                       be_val<ppcaddr_t> *outFreeFn)
{
   *outAllocFn = static_cast<ppcaddr_t>(sMemAlloc);
   *outFreeFn = static_cast<ppcaddr_t>(sMemFree);
   return 0;
}


/**
 * Set the allocator to be used for allocating data sections for dynamically loaded libraries.
 */
int
OSDynLoad_SetTLSAllocator(ppcaddr_t allocFn,
                          ppcaddr_t freeFn)
{
   if (!allocFn || !freeFn) {
      return 0xBAD10017;
   }

   sTLSAlloc = allocFn;
   sTLSFree = freeFn;
   return 0;
}


/**
 * Return the allocators set by OSDynLoad_SetAllocator.
 */
int
OSDynLoad_GetTLSAllocator(be_val<ppcaddr_t> *outAllocFn,
                          be_val<ppcaddr_t> *outFreeFn)
{
   *outAllocFn = static_cast<ppcaddr_t>(sTLSAlloc);
   *outFreeFn = static_cast<ppcaddr_t>(sTLSFree);
   return 0;
}


/**
 * Load and return the handle of a dynamic library.
 *
 * If the library is already loaded then increase ref count.
 */
int
OSDynLoad_Acquire(char const *name,
                  be_ModuleHandle *outHandle)
{
   auto module = kernel::loader::loadRPL(name);

   if (!module) {
      gLog->debug("OSDynLoad_Acquire {} failed", name);
      *outHandle = nullptr;
      return 0xBAD10001;
   }

   // Call the modules entryPoint if it has one
   if (module->entryPoint && !module->entryCalled) {
      gLog->info("Executing module {} rpl_entry", module->name);

      module->entryCalled = true;

      auto moduleStart = kernel::loader::RplEntryPoint(module->entryPoint);
      moduleStart(module->handle, kernel::loader::RplEntryReasonLoad);
   }

   *outHandle = module->handle;
   return 0;
}


/**
 * Find the export from a library handle.
 *
 * Note that for TLS there will be a symbol, but no section,
 * and for normal data / code there with be a section but no symbol.
 */
int
OSDynLoad_FindExport(ModuleHandle handle,
                     int isData,
                     char const *name,
                     be_val<ppcaddr_t> *outAddr)
{
   auto module = handle->ptr;
   auto addr = module->findExport(name);

   *outAddr = addr;

   if (!addr) {
      gLog->debug("OSDynLoad_FindExport export {} not found", name);
      return 0xBAD10001;
   }

   // Let's first try to verify the export type based off the section it is in
   auto section = module->findAddressSection(addr);

   if (section) {
      if (isData) {
         if (section->type != kernel::loader::LoadedSectionType::Data) {
            gLog->debug("OSDynLoad_FindExport export {} expected data, found function", name);
            return 0xBAD10001;
         }
      } else {
         if (section->type != kernel::loader::LoadedSectionType::Code) {
            gLog->debug("OSDynLoad_FindExport export {} expected function, found data", name);
            return 0xBAD10001;
         }
      }

      return 0;
   }

   // Now let's try to verify it based off it's symbol
   auto symbol = module->findSymbol(addr);

   if (symbol) {
      if (symbol->type == kernel::loader::SymbolType::TLS) {
         return 0xBAD10033;
      } else if (isData && symbol->type != kernel::loader::SymbolType::Data) {
         gLog->debug("OSDynLoad_FindExport export {} expected data, found function", name);
         return 0xBAD10001;
      } else if (!isData && symbol->type != kernel::loader::SymbolType::Function) {
         gLog->debug("OSDynLoad_FindExport export {} expected function, found data", name);
         return 0xBAD10001;
      }

      return 0;
   }

   // Couldn't find a section or a symbol for the export, this should never happen...!
   decaf_abort(fmt::format("OSDynLoad_FindExport could not find symbol or section for export address 0x{:08X}", addr));
   return 0xBAD10001;
}


/**
 * Release handle of loaded library.
 *
 * Decreases the ref count of the library pointed to by handle.
 * The library is unloaded if ref count hits 0.
 */
void
OSDynLoad_Release(ModuleHandle handle)
{
   // TODO: Unload library when ref count hits 0
}


/**
 * Get the name of a module from it's handle.
 */
int
OSDynLoad_GetModuleName(ModuleHandle handle,
                        char *buffer,
                        uint32_t size)
{
   kernel::loader::LoadedModule *module = nullptr;

   if (mem::untranslate(handle) == -1) {
      module = kernel::getUserModule();
   } else {
      module = handle->ptr;
   }

   auto length = std::min<size_t>(size - 1, module->name.size());
   strncpy(buffer, module->name.data(), length);
   buffer[length] = 0;
   return 0;
}

void
Module::registerDynLoadFunctions()
{
   RegisterKernelFunction(OSDynLoad_Acquire);
   RegisterKernelFunction(OSDynLoad_FindExport);
   RegisterKernelFunction(OSDynLoad_Release);
   RegisterKernelFunction(OSDynLoad_SetAllocator);
   RegisterKernelFunction(OSDynLoad_GetAllocator);
   RegisterKernelFunction(OSDynLoad_GetModuleName);

   RegisterInternalFunction(dynloadDefaultAlloc, sMemAlloc);
   RegisterInternalFunction(dynloadDefaultFree, sMemFree);

   RegisterInternalFunction(tlsDefaultAlloc, sTLSAlloc);
   RegisterInternalFunction(tlsDefaultFree, sTLSFree);
}

namespace internal
{

/**
 * Wrapper func to call function pointer set by OSDynLoad_SetAllocator
 */
int
dynLoadMemAlloc(int size,
                int alignment,
                void **outPtr)
{
   auto value = coreinit::internal::sysAlloc<be_ptr<void>>();
   auto result = sMemAlloc(size, alignment, value);
   *outPtr = *value;
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


/**
 * Wrapper func to call function pointer set by OSDynLoad_SetTLSAllocator
 */
int
dynLoadTLSAlloc(int size,
                int alignment,
                be_ptr<void> *outPtr)
{
   return sTLSAlloc(size, alignment, outPtr);
}


/**
 * Wrapper func to call function pointer set by OSDynLoad_SetTLSAllocator
 */
void
dynLoadTLSFree(void *addr)
{
   sTLSFree(addr);
}

} // namespace internal

} // namespace coreinit

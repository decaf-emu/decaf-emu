#include <algorithm>
#include "coreinit.h"
#include "coreinit_dynload.h"
#include "coreinit_memheap.h"
#include "coreinit_expheap.h"
#include "interpreter.h"
#include "system.h"

static wfunc_ptr<int, int, int, be_val<uint32_t>*>
pOSDynLoad_MemAlloc;

static wfunc_ptr<void, p32<void>>
pOSDynLoad_MemFree;

template<unsigned N>
static inline uint32_t
OSExecuteCallback(uint32_t addr, uint32_t (&args)[N])
{
   assert(false);
   return 0;
   /*auto state = Thread::getCurrentThread()->getThreadState();
   uint32_t save[N];
   uint32_t result;

   for (auto i = 0u; i < N; ++i) {
      save[i] = state->gpr[3 + i];
      state->gpr[3 + i] = args[i];
   }

   gInterpreter.execute(state, addr);

   result = state->gpr[3];

   for (auto i = 0u; i < N; ++i) {
      state->gpr[3 + i] = save[i];
   }

   return result;*/
}

int MEM_DynLoad_DefaultAlloc(int size, int alignment, be_val<uint32_t> *outPtr)
{
   assert(false);
   return 0;
   /*
   if (!outPtr) {
      return 0xBAD10017;
   }

   if (alignment < 0) {
      alignment = std::min(alignment, -4);
   } else {
      alignment = std::max(alignment, 4);
   }

   uint32_t args[] = {
      static_cast<uint32_t>(size),
      static_cast<uint32_t>(alignment)
   };

   *outPtr = OSExecuteCallback(static_cast<addr_t>(pMEMAllocFromDefaultHeapEx), args);
   return 0;*/
}

void MEM_DynLoad_DefaultFree(p32<void> addr)
{
   assert(false);
   /*
   uint32_t args[] = {
      static_cast<uint32_t>(addr)
   };

   OSExecuteCallback(static_cast<addr_t>(pMEMFreeToDefaultHeap), args);*/
}

int
OSDynLoad_SetAllocator(uint32_t allocFn, uint32_t freeFn)
{
   if (!allocFn || !freeFn) {
      return 0xBAD10017;
   }

   pOSDynLoad_MemAlloc = allocFn;
   pOSDynLoad_MemFree = freeFn;
   return 0;
}

int
OSDynLoad_GetAllocator(be_val<uint32_t> *outAllocFn, be_val<uint32_t> *outFreeFn)
{
   *outAllocFn = static_cast<uint32_t>(pOSDynLoad_MemAlloc);
   *outFreeFn = static_cast<uint32_t>(pOSDynLoad_MemFree);
   return 0;
}

// Wrapper func to call function pointer set by OSDynLoad_SetAllocator
int
OSDynLoad_MemAlloc(int size, int alignment, uint32_t *outPtr)
{
   auto handle = MEMGetBaseHeapHandle(BaseHeapType::MEM2);
   auto memory = MEMAllocFromExpHeapEx(reinterpret_cast<ExpandedHeap*>(handle), size, alignment);
   *outPtr = gMemory.untranslate(memory);
   return 0;

   // TODO: Fix user dynload callback
   /*
   auto state = Thread::getCurrentThread()->getThreadState();
   state->gpr[1] -= 4;

   auto stack = state->gpr[1];

   uint32_t args[] = {
      size,
      alignment,
      stack
   };

   auto result = OSExecuteCallback(pOSDynLoad_MemAlloc->value, args);
   *outPtr = gMemory.read<uint32_t>(stack);
   state->gpr[1] += 4;
   return result;
   */
}

// Wrapper func to call function pointer set by OSDynLoad_SetAllocator
void
OSDynLoad_MemFree(p32<void> addr)
{
   auto handle = MEMGetBaseHeapHandle(BaseHeapType::MEM2);
   return MEMFreeToExpHeap((ExpandedHeap*)(void*)handle, addr);

   // TODO: Fix user dynload callback
   /*
   uint32_t args[] = {
      addr.value
   };

   OSExecuteCallback(pOSDynLoad_MemFree->value, args);
   */
}

int
OSDynLoad_Acquire(char const *name, be_val<uint32_t> *outHandle)
{
   auto module = gSystem.findModule(name);

   if (module) {
      *outHandle = static_cast<uint32_t>(module->getHandle());
      return 0;
   } else {
      *outHandle = 0;
      return 0xBAD10001;
   }
}

int
OSDynLoad_FindExport(LoadedModule *module, int isData, char const *name, be_val<uint32_t> *outAddr)
{
   uint32_t addr = 0;

   if (module->type == LoadedModule::Kernel) {
      auto kmod = reinterpret_cast<KernelModule*>(module->ptr);
      auto exp = kmod->findExport(name);

      if (!exp) {
         gLog->debug("OSDynLoad_FindExport export {} not found", name);
      } else if (exp->type == KernelExport::Data) {
         if (isData == 0) {
            gLog->debug("OSDynLoad_FindExport isData == 0 for data export {}", name);
         } else {
            auto kdata = reinterpret_cast<KernelData *>(exp);
            addr = static_cast<uint32_t>(*kdata->vptr);
         }
      } else if (exp->type == KernelExport::Function) {
         if (isData == 1) {
            gLog->debug("OSDynLoad_FindExport isData == 1 for function export {}", name);
         } else {
            auto kfunc = reinterpret_cast<KernelFunction *>(exp);
            addr = kfunc->vaddr;
         }
      }
   }

   if (addr) {
      *outAddr = addr;
      return 0;
   } else {
      *outAddr = 0;
      return 0xBAD10001;
   }
}

void
OSDynLoad_Release(DynLoadModuleHandle handle)
{
}

void
CoreInit::registerDynLoadFunctions()
{
   RegisterKernelFunction(OSDynLoad_Acquire);
   RegisterKernelFunction(OSDynLoad_FindExport);
   RegisterKernelFunction(OSDynLoad_Release);
   RegisterKernelFunction(OSDynLoad_SetAllocator);
   RegisterKernelFunction(OSDynLoad_GetAllocator);
   RegisterKernelFunction(MEM_DynLoad_DefaultAlloc);
   RegisterKernelFunction(MEM_DynLoad_DefaultFree);
}

void
CoreInit::initialiseDynLoad()
{
   pOSDynLoad_MemAlloc = findExportAddress("MEM_DynLoad_DefaultAlloc");
   pOSDynLoad_MemFree = findExportAddress("MEM_DynLoad_DefaultFree");
}

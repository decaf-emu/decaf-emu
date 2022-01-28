#include "cafe/libraries/nn_ec/nn_ec.h"
#include "cafe/libraries/nn_ec/nn_ec_memorymanager.h"

#include "cafe/cafe_ppc_interface_invoke_guest.h"
#include "cafe/libraries/coreinit/coreinit_memdefaultheap.h"

#include <libcpu/be2_struct.h>
#include <mutex>

namespace cafe::nn_ec
{

struct StaticMemoryManagerData
{
   StaticMemoryManagerData()
   {
      MemoryManager_Constructor(virt_addrof(memoryManager));
   }

   be2_struct<MemoryManager> memoryManager;
};

static virt_ptr<StaticMemoryManagerData> sMemoryManagerData = nullptr;
static MemoryManager::AllocFn sDefaultAllocFn;
static MemoryManager::FreeFn sDefaultFreeFn;

virt_ptr<MemoryManager>
MemoryManager_GetSingleton()
{
   return virt_addrof(sMemoryManagerData->memoryManager);
}

virt_ptr<MemoryManager>
MemoryManager_Constructor(virt_ptr<MemoryManager> self)
{
   self->allocFn = sDefaultAllocFn;
   self->freeFn = sDefaultFreeFn;
   return self;
}

virt_ptr<void>
MemoryManager_Allocate(virt_ptr<MemoryManager> self,
                       uint32_t size,
                       uint32_t align)
{
   std::unique_lock<nn::os::CriticalSection> lock { self->mutex };
   return cafe::invoke(cpu::this_core::state(), self->allocFn, size, align);
}

void
MemoryManager_Free(virt_ptr<MemoryManager> self,
                   virt_ptr<void> ptr)
{
   std::unique_lock<nn::os::CriticalSection> lock { self->mutex };
   return cafe::invoke(cpu::this_core::state(), self->freeFn, ptr);
}

namespace internal
{

void
MemoryManager_SetAllocator(MemoryManager::AllocFn allocFn,
                           MemoryManager::FreeFn freeFn)
{
   auto memoryManager = MemoryManager_GetSingleton();

   std::unique_lock<nn::os::CriticalSection> lock { memoryManager->mutex };
   memoryManager->allocFn = allocFn;
   memoryManager->freeFn = freeFn;
}

static virt_ptr<void> defaultAllocFn(uint32_t size, uint32_t align)
{
   return coreinit::MEMAllocFromDefaultHeapEx(size, align);
}

static void defaultFreeFn(virt_ptr<void> ptr)
{
   coreinit::MEMFreeToDefaultHeap(ptr);
}

} // namespace internal

void
Library::registerMemoryManagerSymbols()
{
   RegisterDataInternal(sMemoryManagerData);

   RegisterFunctionInternal(internal::defaultAllocFn, sDefaultAllocFn);
   RegisterFunctionInternal(internal::defaultFreeFn, sDefaultFreeFn);
}

} // namespace cafe::nn_ec

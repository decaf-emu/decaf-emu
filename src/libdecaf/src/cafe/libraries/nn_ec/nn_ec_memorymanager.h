#pragma once
#include "cafe/nn/cafe_nn_os_criticalsection.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_ec
{

struct MemoryManager
{
   using AllocFn = virt_func_ptr<virt_ptr<void>(uint32_t size, uint32_t align)>;
   using FreeFn = virt_func_ptr<void(virt_ptr<void> ptr)>;

   be2_struct<nn::os::CriticalSection> mutex;
   be2_val<AllocFn> allocFn;
   be2_val<FreeFn> freeFn;
   UNKNOWN(0x48 - 0x34);
};
CHECK_OFFSET(MemoryManager, 0x00, mutex);
CHECK_OFFSET(MemoryManager, 0x2C, allocFn);
CHECK_OFFSET(MemoryManager, 0x30, freeFn);
CHECK_SIZE(MemoryManager, 0x48);

virt_ptr<MemoryManager>
MemoryManager_GetSingleton();

virt_ptr<MemoryManager>
MemoryManager_Constructor(virt_ptr<MemoryManager> self);

virt_ptr<void>
MemoryManager_Allocate(virt_ptr<MemoryManager> self,
                       uint32_t size,
                       uint32_t align);

void
MemoryManager_Free(virt_ptr<MemoryManager> self,
                   virt_ptr<void> ptr);

namespace internal
{

void
MemoryManager_SetAllocator(MemoryManager::AllocFn allocFn,
                           MemoryManager::FreeFn freeFn);

} // internal

} // namespace cafe::nn_ec

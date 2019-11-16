#include "nn_nim.h"
#include "nn_nim_client.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/coreinit/coreinit_mutex.h"
#include "cafe/nn/cafe_nn_ipc_client.h"

using namespace cafe::coreinit;

namespace cafe::nn_nim
{

struct StaticClientData
{
   StaticClientData()
   {
      OSInitMutex(virt_addrof(mutex));
   }

   alignas(64) be2_array<uint8_t, 0x10000> allocatorMemory;

   be2_struct<OSMutex> mutex;
   be2_val<uint32_t> refCount = 0u;
   be2_struct<nn::ipc::Client> client;
   be2_struct<nn::ipc::BufferAllocator> allocator;
};

static virt_ptr<StaticClientData> sClientData = nullptr;

nn::Result
Initialize()
{
   OSLockMutex(virt_addrof(sClientData->mutex));
   if (sClientData->refCount == 0) {
      sClientData->client.initialise(make_stack_string("/dev/nim"));
      sClientData->allocator.initialise(virt_addrof(sClientData->allocatorMemory),
                                        sClientData->allocatorMemory.size());
   }

   sClientData->refCount++;
   OSUnlockMutex(virt_addrof(sClientData->mutex));
   return nn::ResultSuccess;
}

void
Finalize()
{
   OSLockMutex(virt_addrof(sClientData->mutex));
   if (sClientData->refCount > 0) {
      sClientData->refCount--;

      if (sClientData->refCount == 0) {
         sClientData->client.close();
      }
   }
   OSUnlockMutex(virt_addrof(sClientData->mutex));
}

namespace internal
{

virt_ptr<nn::ipc::Client>
getClient()
{
   return virt_addrof(sClientData->client);
}

virt_ptr<nn::ipc::BufferAllocator>
getAllocator()
{
   return virt_addrof(sClientData->allocator);
}

} // namespace internal

void
Library::registerClientSymbols()
{
   RegisterFunctionExportName("Initialize__Q2_2nn3nimFv", Initialize);
   RegisterFunctionExportName("Finalize__Q2_2nn3nimFv", Finalize);

   RegisterDataInternal(sClientData);
}

}  // namespace cafe::nn_nim

#include "nn_act.h"
#include "nn_act_client.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/coreinit/coreinit_mutex.h"
#include "cafe/libraries/coreinit/coreinit_systeminfo.h"
#include "cafe/nn/cafe_nn_ipc_client.h"
#include "nn/act/nn_act_result.h"

using namespace cafe::coreinit;
using namespace nn::act;

namespace cafe::nn_act
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
      sClientData->client.initialise(make_stack_string("/dev/act"));
      sClientData->allocator.initialise(virt_addrof(sClientData->allocatorMemory),
                                        sClientData->allocatorMemory.size());

   }

   sClientData->refCount++;
   OSUnlockMutex(virt_addrof(sClientData->mutex));
   return nn::ResultSuccess;
}

nn::Result
Finalize()
{
   auto result = nn::ResultSuccess;
   OSLockMutex(virt_addrof(sClientData->mutex));
   if (sClientData->refCount > 0) {
      sClientData->refCount--;

      if (sClientData->refCount == 0) {
         sClientData->client.close();
      }
   } else {
      result = 0xC070FA80;
   }
   OSUnlockMutex(virt_addrof(sClientData->mutex));
   return result;
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
   RegisterFunctionExportName("Initialize__Q2_2nn3actFv", Initialize);
   RegisterFunctionExportName("Finalize__Q2_2nn3actFv", Finalize);

   RegisterDataInternal(sClientData);
}

}  // namespace cafe::nn_act

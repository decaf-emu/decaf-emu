#include "nn_ac.h"
#include "nn_ac_client.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/coreinit/coreinit_mutex.h"
#include "cafe/libraries/coreinit/coreinit_systeminfo.h"
#include "cafe/nn/cafe_nn_ipc_client.h"
#include "nn/ac/nn_ac_service.h"

using namespace cafe::coreinit;
using namespace nn::ac;
using namespace nn::ipc;

namespace cafe::nn_ac
{

struct StaticClientData
{
   StaticClientData()
   {
      OSInitMutex(virt_addrof(mutex));
   }

   alignas(256) be2_array<uint8_t, 0x10000> allocatorMemory;

   be2_struct<OSMutex> mutex;
   be2_val<uint32_t> refCount = 0u;
   be2_struct<nn::ipc::Client> client;
   be2_struct<nn::ipc::BufferAllocator> allocator;
};

static virt_ptr<StaticClientData> sClientData = nullptr;

nn::Result
Initialize()
{
   auto result = nn::ResultSuccess;
   OSLockMutex(virt_addrof(sClientData->mutex));
   if (sClientData->refCount == 0) {
      result = sClientData->client.initialise(make_stack_string("/dev/ac_main"));
      sClientData->allocator.initialise(virt_addrof(sClientData->allocatorMemory),
                                        sClientData->allocatorMemory.size());
      if (result) {
         internal::getClient()->sendSyncRequest(
            ClientCommand<services::AcService::Initialise> { internal::getAllocator() });
      }
   }

   sClientData->refCount++;
   OSUnlockMutex(virt_addrof(sClientData->mutex));
   return result;
}

void
Finalize()
{
   OSLockMutex(virt_addrof(sClientData->mutex));
   if (sClientData->refCount > 0) {
      sClientData->refCount--;

      if (sClientData->refCount == 0) {
         internal::getClient()->sendSyncRequest(
            ClientCommand<services::AcService::Finalise> { internal::getAllocator() });
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
   RegisterFunctionExportName("Initialize__Q2_2nn2acFv",
                              Initialize);
   RegisterFunctionExportName("Finalize__Q2_2nn2acFv",
                              Finalize);

   RegisterDataInternal(sClientData);
}

}  // namespace cafe::nn_ac

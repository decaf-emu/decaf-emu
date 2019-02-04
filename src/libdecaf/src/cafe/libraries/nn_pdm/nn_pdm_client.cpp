#include "nn_pdm.h"
#include "nn_pdm_client.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/coreinit/coreinit_mutex.h"
#include "cafe/libraries/coreinit/coreinit_systeminfo.h"
#include "cafe/nn/cafe_nn_ipc_client.h"
#include "nn/pdm/nn_pdm_result.h"

using namespace cafe::coreinit;
using namespace nn::pdm;

namespace cafe::nn_pdm
{

struct StaticClientData
{
   StaticClientData()
   {
      OSInitMutex(virt_addrof(mutex));
   }

   alignas(64) be2_array<uint8_t, 4096> allocatorMemory;

   be2_struct<OSMutex> mutex;
   be2_val<uint32_t> refCount = 0u;
   be2_struct<nn::ipc::Client> client;
   be2_struct<nn::ipc::BufferAllocator> allocator;
};

static virt_ptr<StaticClientData> sClientData = nullptr;

nn::Result
PDMInitialize()
{
   OSLockMutex(virt_addrof(sClientData->mutex));
   if (sClientData->refCount == 0) {
      sClientData->client.initialise(make_stack_string("/dev/pdm"));
      sClientData->allocator.initialise(virt_addrof(sClientData->allocatorMemory),
                                        sClientData->allocatorMemory.size());
   }

   sClientData->refCount++;
   OSUnlockMutex(virt_addrof(sClientData->mutex));
   return ResultSuccess;
}

void
PDMFinalize()
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
   RegisterFunctionExport(PDMInitialize);
   RegisterFunctionExport(PDMFinalize);

   RegisterFunctionExportName("Initialize__Q2_2nn3pdmFv", PDMInitialize);
   RegisterFunctionExportName("Finalize__Q2_2nn3pdmFv", PDMFinalize);

   RegisterDataInternal(sClientData);
}

}  // namespace cafe::nn_pdm

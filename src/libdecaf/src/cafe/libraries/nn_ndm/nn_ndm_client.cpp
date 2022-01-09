#include "nn_ndm.h"
#include "nn_ndm_client.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/coreinit/coreinit_mutex.h"
#include "cafe/nn/cafe_nn_ipc_client.h"

using namespace cafe::coreinit;

namespace cafe::nn_ndm
{

struct StaticClientData
{
   StaticClientData()
   {
      OSInitMutex(virt_addrof(mutex));
   }

   alignas(64) be2_array<uint8_t, 0x1000> allocatorMemory;

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
      sClientData->client.initialise(make_stack_string("/dev/ndm"));
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
   OSLockMutex(virt_addrof(sClientData->mutex));
   if (sClientData->refCount > 0) {
      sClientData->refCount--;

      if (sClientData->refCount == 0) {
         sClientData->client.close();
      }
   }
   OSUnlockMutex(virt_addrof(sClientData->mutex));
}

bool
IsInitialized()
{
   // TODO: This should actuallly be sClientData->client.isInitialised();
   //       however we have not implemented /dev/ndm in IOS
   return sClientData->refCount > 0;
}

nn::Result
EnableResumeDaemons()
{
   decaf_warn_stub();
   return nn::ResultSuccess;
}

nn::Result
GetDaemonStatus(virt_ptr<uint32_t> status, // nn::ndm::IDaemon::Status *
                uint32_t unknown) // nn::ndm::Cafe::DaemonName
{
   decaf_warn_stub();
   *status = 3u;
   return nn::ResultSuccess;
}

void
Library::registerClientSymbols()
{
   RegisterFunctionExportName("Initialize__Q2_2nn3ndmFv",
                              Initialize);
   RegisterFunctionExportName("Finalize__Q2_2nn3ndmFv",
                              Finalize);
   RegisterFunctionExportName("IsInitialized__Q2_2nn3ndmFv",
                              IsInitialized);
   RegisterFunctionExportName("EnableResumeDaemons__Q2_2nn3ndmFv",
                              EnableResumeDaemons);
   RegisterFunctionExportName("GetDaemonStatus__Q2_2nn3ndmFPQ4_2nn3ndm7IDaemon6StatusQ4_2nn3ndm4Cafe10DaemonName",
                              GetDaemonStatus);

   RegisterFunctionExportName("NDMInitialize", Initialize);
   RegisterFunctionExportName("NDMFinalize", Finalize);
   RegisterFunctionExportName("NDMEnableResumeDaemons", EnableResumeDaemons);

   RegisterDataInternal(sClientData);
}

} // namespace cafe::nn_ndm

#include "nn_boss.h"
#include "nn_boss_client.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/coreinit/coreinit_mutex.h"
#include "cafe/libraries/coreinit/coreinit_systeminfo.h"
#include "cafe/nn/cafe_nn_ipc_client.h"

using namespace cafe::coreinit;
using namespace nn::ipc;

namespace cafe::nn_boss
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
      result = sClientData->client.initialise(make_stack_string("/dev/boss"));
      sClientData->allocator.initialise(virt_addrof(sClientData->allocatorMemory),
                                        sClientData->allocatorMemory.size());
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
         sClientData->client.close();
      }
   }
   OSUnlockMutex(virt_addrof(sClientData->mutex));
}

bool
IsInitialized()
{
   return sClientData->client.isInitialised();
}

BossState
GetBossState()
{
   decaf_warn_stub();
   return BossState::Unknown0;
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
   RegisterFunctionExportName("Initialize__Q2_2nn4bossFv",
                              Initialize);
   RegisterFunctionExportName("Finalize__Q2_2nn4bossFv",
                              Finalize);
   RegisterFunctionExportName("IsInitialized__Q2_2nn4bossFv",
                              IsInitialized);
   RegisterFunctionExportName("GetBossState__Q2_2nn4bossFv",
                              GetBossState);

   RegisterDataInternal(sClientData);
}

}  // namespace cafe::nn_boss

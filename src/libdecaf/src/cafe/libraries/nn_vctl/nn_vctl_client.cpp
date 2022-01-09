#include "nn_vctl.h"
#include "nn_vctl_client.h"

#include "cafe/libraries/coreinit/coreinit_mutex.h"

using namespace cafe::coreinit;

namespace cafe::nn_vctl
{

struct StaticClientData
{
   StaticClientData()
   {
      OSInitMutex(virt_addrof(mutex));
   }

   be2_struct<OSMutex> mutex;
   be2_val<uint32_t> refCount = 0u;
   be2_val<BOOL> initialised = FALSE;
};

static virt_ptr<StaticClientData> sClientData = nullptr;

nn::Result
Initialize()
{
   OSLockMutex(virt_addrof(sClientData->mutex));
   if (sClientData->refCount == 0) {
      sClientData->initialised = TRUE;
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
         sClientData->initialised = FALSE;
      }
   }
   OSUnlockMutex(virt_addrof(sClientData->mutex));
}

void
Library::registerClientSymbols()
{
   RegisterFunctionExportName("Initialize__Q2_2nn4vctlFv", Initialize);
   RegisterFunctionExportName("Finalize__Q2_2nn4vctlFv", Finalize);

   RegisterDataInternal(sClientData);
}

}  // namespace cafe::nn_vctl

#include "nn_spm.h"
#include "nn_spm_client.h"
#include "nn_spm_extendedstorageservice.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/nn/cafe_nn_ipc_client.h"
#include "nn/spm/nn_spm_extendedstorageservice.h"

using namespace nn::spm;
using namespace nn::ipc;

namespace cafe::nn_spm
{

using nn::spm::services::ExtendedStorageService;

nn::Result
SetAutoFatal(uint8_t autoFatal)
{
   auto command = ClientCommand<ExtendedStorageService::SetAutoFatal> { internal::getAllocator() };
   command.setParameters(autoFatal);
   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.ok()) {
      result = command.readResponse();
   }

   return result;
}

void
Library::registerExtendedStorageServiceSymbols()
{
   RegisterFunctionExportName("SetAutoFatal__Q2_2nn3spmFb", SetAutoFatal);
}

}  // namespace cafe::nn_spm

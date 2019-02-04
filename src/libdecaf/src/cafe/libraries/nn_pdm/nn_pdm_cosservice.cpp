#include "nn_pdm.h"
#include "nn_pdm_client.h"
#include "nn_pdm_cosservice.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/nn/cafe_nn_ipc_client.h"
#include "nn/pdm/nn_pdm_result.h"
#include "nn/pdm/nn_pdm_cosservice.h"

using namespace nn::ipc;
using namespace nn::pdm;

namespace cafe::nn_pdm
{

nn::Result
PDMGetPlayDiaryMaxLength(virt_ptr<uint32_t> outMaxLength)
{
   auto command = ClientCommand<services::CosService::GetPlayDiaryMaxLength> { internal::getAllocator() };
   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.failed()) {
      return result;
   }

   auto maxLength = uint32_t { 0 };
   result = command.readResponse(maxLength);
   if (result.ok()) {
      *outMaxLength = maxLength;
   }

   return result;
}

nn::Result
PDMGetPlayStatsMaxLength(virt_ptr<uint32_t> outMaxLength)
{
   auto command = ClientCommand<services::CosService::GetPlayStatsMaxLength> { internal::getAllocator() };
   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.failed()) {
      return result;
   }

   auto maxLength = uint32_t { 0 };
   result = command.readResponse(maxLength);
   if (result.ok()) {
      *outMaxLength = maxLength;
   }

   return result;
}

void
Library::registerCosServiceSymbols()
{
   RegisterFunctionExportName("GetPlayDiaryMaxLength__Q2_2nn3pdmFPi",
                              PDMGetPlayDiaryMaxLength);
   RegisterFunctionExportName("GetPlayStatsMaxLength__Q2_2nn3pdmFPi",
                              PDMGetPlayStatsMaxLength);
}

}  // namespace cafe::nn_pdm

#include "nn_acp.h"
#include "nn_acp_client.h"
#include "nn_acp_miscservice.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/nn/cafe_nn_ipc_client.h"
#include "nn/acp/nn_acp_result.h"
#include "nn/acp/nn_acp_miscservice.h"

using namespace nn::acp;
using namespace nn::ipc;

namespace cafe::nn_acp
{

//! 01/01/2000 @ 12:00am
static constexpr auto NetworkTimeEpoch = 946684800000000;

/**
 * Sets outTime to microseconds since our NetworkTimeEpoch - 01/01/2000
 */
nn::Result
ACPGetNetworkTime(virt_ptr<int64_t> outTime,
                  virt_ptr<uint32_t> outUnknown)
{
   auto command = ClientCommand<services::MiscService::GetNetworkTime> { internal::getAllocator() };
   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.failed()) {
      return result;
   }

   auto time = int64_t { 0 };
   auto unk = uint32_t { 0 };
   result = command.readResponse(time, unk);
   if (result.ok()) {
      *outTime = time - NetworkTimeEpoch;
      *outUnknown = unk;
   }

   return result;
}

nn::Result
ACPGetTitleIdOfMainApplication(virt_ptr<ACPTitleId> outTitleId)
{
   auto command = ClientCommand<services::MiscService::GetTitleIdOfMainApplication> { internal::getAllocator() };
   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.failed()) {
      return result;
   }

   auto titleId = ACPTitleId { 0 };
   result = command.readResponse(titleId);
   if (result.ok()) {
      *outTitleId = titleId;
   }

   return result;
}

nn::Result
ACPGetTitleMetaXml(ACPTitleId titleId,
                   virt_ptr<ACPMetaXml> outData)
{
   auto command = ClientCommand<services::MiscService::GetTitleMetaXml> { internal::getAllocator() };
   command.setParameters(outData, titleId);
   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.failed()) {
      return result;
   }

   return command.readResponse();
}

void
Library::registerMiscServiceSymbols()
{
   RegisterFunctionExport(ACPGetNetworkTime);
   RegisterFunctionExport(ACPGetTitleIdOfMainApplication);
   RegisterFunctionExport(ACPGetTitleMetaXml);
   RegisterFunctionExportName("GetTitleMetaXml__Q2_2nn3acpFULP11_ACPMetaXml",
                              ACPGetTitleMetaXml);
}

}  // namespace cafe::nn_acp

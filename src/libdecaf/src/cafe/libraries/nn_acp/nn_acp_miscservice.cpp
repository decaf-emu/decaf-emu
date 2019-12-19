#include "nn_acp.h"
#include "nn_acp_client.h"
#include "nn_acp_miscservice.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/coreinit/coreinit_time.h"
#include "cafe/nn/cafe_nn_ipc_client.h"
#include "nn/acp/nn_acp_result.h"
#include "nn/acp/nn_acp_miscservice.h"

#include <chrono>
#include <common/platform_time.h>

using namespace nn::acp;
using namespace nn::ipc;

namespace cafe::nn_acp
{

//! Microseconds for Unix-epoch timestamp 01/01/2000 @ 12:00am
static constexpr auto NetworkTimeEpoch = 946684800000000;


/**
 * Convert network time to calendar time.
 *
 * networkTime is microseconds since NetworkTimeEpoch.
 */
void
ACPConvertNetworkTimeToOSCalendarTime(int64_t networkTime,
                                      virt_ptr<coreinit::OSCalendarTime> calendarTime)
{
   auto time = std::chrono::microseconds(networkTime) + std::chrono::microseconds(NetworkTimeEpoch);

   auto tm = platform::localtime(std::chrono::duration_cast<std::chrono::seconds>(time).count());
   calendarTime->tm_sec = tm.tm_sec;
   calendarTime->tm_min = tm.tm_min;
   calendarTime->tm_hour = tm.tm_hour;
   calendarTime->tm_mday = tm.tm_mday;
   calendarTime->tm_mon = tm.tm_mon;
   calendarTime->tm_year = tm.tm_year + 1900; // posix tm_year is year - 1900
   calendarTime->tm_wday = tm.tm_wday;
   calendarTime->tm_yday = tm.tm_yday;

   auto timeOffset = time - std::chrono::duration_cast<std::chrono::seconds>(time);
   auto msOffset = std::chrono::duration_cast<std::chrono::milliseconds>(timeOffset);
   auto uOffset = std::chrono::duration_cast<std::chrono::microseconds>(timeOffset - msOffset);
   calendarTime->tm_msec = static_cast<int32_t>(msOffset.count());
   calendarTime->tm_usec = static_cast<int32_t>(uOffset.count());
}


/**
 * Sets outTime to microseconds since NetworkTimeEpoch
 */
ACPResult
ACPGetNetworkTime(virt_ptr<int64_t> outTime,
                  virt_ptr<uint32_t> outUnknown)
{
   auto command = ClientCommand<services::MiscService::GetNetworkTime> { internal::getAllocator() };
   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.ok()) {
      auto time = int64_t { 0 };
      auto unk = uint32_t { 0 };
      result = command.readResponse(time, unk);
      if (result.ok()) {
         *outTime = time - NetworkTimeEpoch;
         *outUnknown = unk;
      }
   }

   return ACPConvertToACPResult(result, "ACPGetNetworkTime", 79);
}

ACPResult
ACPGetTitleIdOfMainApplication(virt_ptr<ACPTitleId> outTitleId)
{
   auto command = ClientCommand<services::MiscService::GetTitleIdOfMainApplication> { internal::getAllocator() };
   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.ok()) {
      auto titleId = ACPTitleId{ 0 };
      result = command.readResponse(titleId);
      if (result.ok()) {
         *outTitleId = titleId;
      }
   }

   return ACPConvertToACPResult(result, "GetTitleIdOfMainApplication", 126);
}

ACPResult
ACPGetTitleMetaXml(ACPTitleId titleId,
                   virt_ptr<ACPMetaXml> outData)
{
   auto command = ClientCommand<services::MiscService::GetTitleMetaXml> { internal::getAllocator() };
   command.setParameters(outData, titleId);
   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.ok()) {
      result = command.readResponse();
   }

   return ACPConvertToACPResult(result, "GetTitleMetaXml", 40);
}

void
Library::registerMiscServiceSymbols()
{
   RegisterFunctionExport(ACPConvertNetworkTimeToOSCalendarTime);
   RegisterFunctionExport(ACPGetNetworkTime);
   RegisterFunctionExport(ACPGetTitleIdOfMainApplication);
   RegisterFunctionExport(ACPGetTitleMetaXml);
   RegisterFunctionExportName("GetTitleMetaXml__Q2_2nn3acpFULP11_ACPMetaXml",
                              ACPGetTitleMetaXml);
}

}  // namespace cafe::nn_acp

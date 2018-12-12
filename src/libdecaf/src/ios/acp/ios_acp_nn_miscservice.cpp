#include "ios_acp_nn_miscservice.h"
#include "ios_acp_metaxml.h"

#include "ios/nn/ios_nn_ipc_server_command.h"
#include "nn/acp/nn_acp_result.h"

#include <chrono>
#include <ctime>

using namespace nn::acp;
using namespace nn::ipc;

namespace ios::acp::internal
{

static nn::Result
getNetworkTime(CommandHandlerArgs &args)
{
   auto command = ServerCommand<MiscService::GetNetworkTime> { args };
   auto networkTime = std::chrono::microseconds(time(NULL)).count();
   auto unkValue = 1;
   command.WriteResponse(networkTime, unkValue);
   return ResultSuccess;
}

static nn::Result
getTitleIdOfMainApplication(CommandHandlerArgs &args)
{
   auto command = ServerCommand<MiscService::GetTitleIdOfMainApplication> { args };
   auto buffer = OutBuffer<ACPMetaXml> { };
   auto titleId = ACPTitleId { 0 };

   // TODO: Cache ACPMetaXML of currently loaded title.
   // TODO: Read title_id from meta.xml?

   command.WriteResponse(titleId);
   return ResultSuccess;
}

static nn::Result
getTitleMetaXml(CommandHandlerArgs &args)
{
   auto command = ServerCommand<MiscService::GetTitleMetaXml> { args };
   auto buffer = OutBuffer<ACPMetaXml> { };
   auto titleId = ACPTitleId { 0 };
   command.ReadRequest(buffer, titleId);

   // The whole meta xml should be aligned
   decaf_check(buffer.unalignedAfterBufferSize == 0);
   decaf_check(buffer.unalignedBeforeBufferSize == 0);
   auto metaXml = phys_cast<ACPMetaXml *>(buffer.alignedBuffer);

   // TODO: Use titleId to decide which meta.xml file to read...

   return loadMetaXMLFromPath("/vol/meta/meta.xml", metaXml);
}

nn::Result
MiscService::commandHandler(uint32_t unk1,
                            CommandId command,
                            CommandHandlerArgs &args)
{
   switch (command) {
   case GetNetworkTime::command:
      return getNetworkTime(args);
   case GetTitleIdOfMainApplication::command:
      return getTitleIdOfMainApplication(args);
   case GetTitleMetaXml::command:
      return getTitleMetaXml(args);
   default:
      return ResultSuccess;
   }
}

} // namespace ios::acp::internal

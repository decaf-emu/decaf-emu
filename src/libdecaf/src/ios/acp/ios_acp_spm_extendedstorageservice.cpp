#include "ios_acp_spm_extendedstorageservice.h"

#include "ios/nn/ios_nn_ipc_server_command.h"
#include "nn/nn_result.h"

using namespace nn::spm;
using namespace nn::ipc;

namespace ios::acp::internal
{

static nn::Result
setAutoFatal(CommandHandlerArgs &args)
{
   auto command = ServerCommand<ExtendedStorageService::SetAutoFatal> { args };
   auto autoFatal = uint8_t { 0 };
   command.ReadRequest(autoFatal);
   return nn::ResultSuccess;
}


nn::Result
ExtendedStorageService::commandHandler(uint32_t unk1,
                                       CommandId command,
                                       CommandHandlerArgs &args)
{
   switch (command) {
   case SetAutoFatal::command:
      return setAutoFatal(args);
   default:
      return nn::ResultSuccess;
   }
}

} // namespace ios::acp::internal

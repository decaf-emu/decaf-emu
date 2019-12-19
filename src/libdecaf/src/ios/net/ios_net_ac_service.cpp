#include "ios_net_ac_service.h"

#include "ios/nn/ios_nn_ipc_server_command.h"
#include "nn/ac/nn_ac_result.h"

#include <chrono>
#include <ctime>
#include <uv.h>

using namespace nn::ac;
using namespace nn::ipc;

namespace ios::net::internal
{

static nn::Result
initialise(CommandHandlerArgs &args)
{
   auto command = ServerCommand<AcService::Initialise> { args };
   return nn::ResultSuccess;
}

static nn::Result
finalise(CommandHandlerArgs &args)
{
   auto command = ServerCommand<AcService::Finalise> { args };
   return nn::ResultSuccess;
}

static nn::Result
getAssignedAddress(CommandHandlerArgs &args)
{
   auto command = ServerCommand<AcService::GetAssignedAddress> { args };
   auto unkParamater = uint32_t {};
   command.ReadRequest(unkParamater);

   auto ipAddress = static_cast<uint32_t>((127u << 24) | (0u << 16) | (0u << 8) | 1u);
   command.WriteResponse(ipAddress);
   return nn::ResultSuccess;
}

nn::Result
AcService::commandHandler(uint32_t unk1,
                          CommandId command,
                          CommandHandlerArgs &args)
{
   switch (command) {
   case Initialise::command:
      return initialise(args);
   case Finalise::command:
      return finalise(args);
   case GetAssignedAddress::command:
      return getAssignedAddress(args);
   default:
      return nn::ResultSuccess;
   }
}

} // namespace ios::net::internal

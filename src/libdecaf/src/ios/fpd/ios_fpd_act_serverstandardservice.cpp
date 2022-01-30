#include "ios_fpd_act_serverstandardservice.h"

#include "ios/nn/ios_nn_ipc_server_command.h"
#include "nn/act/nn_act_result.h"
#include "nn/ipc/nn_ipc_result.h"

using namespace nn::act;

using nn::ipc::CommandHandlerArgs;
using nn::ipc::CommandId;
using nn::ipc::OutBuffer;
using nn::ipc::ServerCommand;

namespace ios::fpd::internal
{

static nn::Result
acquireNexServiceToken(CommandHandlerArgs &args)
{
   return ResultUcError;
};


static nn::Result
cancel(CommandHandlerArgs &args)
{
   auto command = ServerCommand<ActServerStandardService::Cancel> { args };
   return nn::ResultSuccess;
}

nn::Result
ActServerStandardService::commandHandler(uint32_t unk1,
                                         CommandId command,
                                         CommandHandlerArgs &args)
{
   switch (command) {
   case Cancel::command:
      return cancel(args);
   case AcquireNexServiceToken::command:
      return acquireNexServiceToken(args);
   default:
      return nn::ipc::ResultInvalidMethodTag;
   }
}

} // namespace ios::fpd::internal

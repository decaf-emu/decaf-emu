#include "ios_fpd_act_accountloaderservice.h"

#include "ios/nn/ios_nn_ipc_server_command.h"
#include "nn/act/nn_act_result.h"
#include "nn/ipc/nn_ipc_result.h"

#include <common/decaf_assert.h>

using namespace nn::act;

using nn::ipc::CommandHandlerArgs;
using nn::ipc::CommandId;
using nn::ipc::OutBuffer;
using nn::ipc::ServerCommand;

namespace ios::fpd::internal
{

static nn::Result
loadConsoleAccount(CommandHandlerArgs &args)
{
   auto command = ServerCommand<ActAccountLoaderService::LoadConsoleAccount> { args };
   /*
   TODO: nn::act::LoadConsoleAccount
   Not really sure what this does tbh.
   */
   return nn::ResultSuccess;
}

nn::Result
ActAccountLoaderService::commandHandler(uint32_t unk1,
                                        CommandId command,
                                        CommandHandlerArgs &args)
{
   switch (command) {
   case LoadConsoleAccount::command:
      return loadConsoleAccount(args);
   default:
      return nn::ipc::ResultInvalidMethodTag;
   }
}

} // namespace ios::fpd::internal

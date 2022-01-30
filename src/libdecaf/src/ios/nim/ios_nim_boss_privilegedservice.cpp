#include "ios_nim_boss_privilegedservice.h"

#include "ios/nn/ios_nn_ipc_server_command.h"
#include "nn/boss/nn_boss_result.h"
#include "nn/ipc/nn_ipc_result.h"

using namespace nn::boss;

using nn::ipc::CommandHandlerArgs;
using nn::ipc::CommandId;
using nn::ipc::OutBuffer;
using nn::ipc::ServerCommand;

namespace ios::nim::internal
{

static nn::Result
addAccount(CommandHandlerArgs &args)
{
   auto command = ServerCommand<PrivilegedService::AddAccount> { args };
   auto persistentId = PersistentId { 0 };
   command.ReadRequest(persistentId);

   // TODO: Implement nn::boss::PrivilegedService::AddAccount

   return ResultSuccess;
}

nn::Result
PrivilegedService::commandHandler(uint32_t unk1,
                                  CommandId command,
                                  CommandHandlerArgs &args)
{
   switch (command) {
   case AddAccount::command:
      return addAccount(args);
   default:
      return nn::ipc::ResultInvalidMethodTag;
   }
}

} // namespace ios::nim::internal

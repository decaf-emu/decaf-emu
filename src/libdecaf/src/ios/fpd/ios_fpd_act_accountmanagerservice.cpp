#include "ios_fpd_act_accountmanagerservice.h"

#include "ios/nn/ios_nn_ipc_server_command.h"
#include "nn/act/nn_act_result.h"

using namespace nn::ipc;
using namespace nn::act;

namespace ios::fpd::internal
{

static nn::Result
createConsoleAccount(CommandHandlerArgs &args)
{
   auto command = ServerCommand<ActAccountManagerService::CreateConsoleAccount> { args };

   /*
   TODO: nn::act::CreateConsoleAccount
   This seems to just do the same than our createAccount method in
   ios_fps_clientstandardservice.cpp, but that seems weird to me as it does not
   return the id or slot of the account created, just an error code. Maybe
   there is some special distinguishment for _Console_ account which we have
   yet to figure out?
   */

   return nn::ResultSuccess;
}

nn::Result
ActAccountManagerService::commandHandler(uint32_t unk1,
                                         CommandId command,
                                         CommandHandlerArgs &args)
{
   switch (command) {
   case CreateConsoleAccount::command:
      return createConsoleAccount(args);
   default:
      return nn::ResultSuccess;
   }
}

} // namespace ios::fpd::internal

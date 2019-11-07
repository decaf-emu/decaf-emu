#include "ios_fpd_act_accountdata.h"
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
   auto account = createAccount();
   if (!account) {
      return ResultSLOTS_FULL;
   }

   account->isCommitted = uint8_t { 0 };
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

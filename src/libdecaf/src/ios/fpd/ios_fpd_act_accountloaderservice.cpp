#include "ios_fpd_act_accountloaderservice.h"
#include "ios_fpd_act_accountdata.h"

#include "ios/nn/ios_nn_ipc_server_command.h"
#include "nn/act/nn_act_result.h"
#include "nn/ipc/nn_ipc_result.h"

#include <common/decaf_assert.h>

using namespace nn::act;

using nn::ipc::CommandHandlerArgs;
using nn::ipc::CommandId;
using nn::ipc::InBuffer;
using nn::ipc::ServerCommand;

namespace ios::fpd::internal
{

static nn::Result
loadConsoleAccount(CommandHandlerArgs &args)
{
   auto command = ServerCommand<ActAccountLoaderService::LoadConsoleAccount> { args };
   auto slotNo = SlotNo { 0 };
   auto loadOption = ACTLoadOption { };
   auto unkInBuffer = InBuffer<const char> { };
   auto unkBool = bool { false };
   command.ReadRequest(slotNo, loadOption, unkInBuffer, unkBool);

   auto account = getAccountBySlotNo(slotNo);
   if (!account) {
      return ResultAccountNotFound;
   }

   /*
   TODO: nn::act::LoadConsoleAccount
   Not really sure what this does tbh.
   */

   setCurrentAccount(account);
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

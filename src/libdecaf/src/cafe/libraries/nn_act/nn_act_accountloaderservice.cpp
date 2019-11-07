#include "nn_act.h"
#include "nn_act_client.h"
#include "nn_act_accountloaderservice.h"

#include "cafe/nn/cafe_nn_ipc_client.h"
#include "nn/act/nn_act_accountloaderservice.h"
#include "nn/act/nn_act_result.h"

#include <common/strutils.h>
#include <chrono>

using namespace nn::act;
using namespace nn::ipc;

namespace cafe::nn_act
{

nn::Result
LoadConsoleAccount(SlotNo slot,
                   ACTLoadOption loadOption,
                   virt_ptr<const char> arg3,
                   bool arg4)
{
   auto command = ClientCommand<services::AccountLoaderService::LoadConsoleAccount> { internal::getAllocator() };
   if (arg3) {
      auto arg3Size = static_cast<uint32_t>(strnlen(arg3.get(), 16) + 1);
      if (arg3Size > 0x11) {
         return ResultInvalidSize;
      }

      command.setParameters(slot, loadOption, { arg3, arg3Size }, !!arg4);
   } else {
      command.setParameters(slot, loadOption, { nullptr, 0 }, !!arg4);
   }

   return internal::getClient()->sendSyncRequest(command);
}

void
Library::registerAccountLoaderServiceSymbols()
{
   RegisterFunctionExportName("LoadConsoleAccount__Q2_2nn3actFUc13ACTLoadOptionPCcb",
                              LoadConsoleAccount);
}

}  // namespace cafe::nn_act

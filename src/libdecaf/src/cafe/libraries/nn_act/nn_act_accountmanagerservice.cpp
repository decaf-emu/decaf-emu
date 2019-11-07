#include "nn_act.h"
#include "nn_act_client.h"
#include "nn_act_accountmanagerservice.h"

#include "cafe/nn/cafe_nn_ipc_client.h"
#include "nn/act/nn_act_accountmanagerservice.h"

#include <chrono>

using namespace nn::act;
using namespace nn::ipc;

namespace cafe::nn_act
{

nn::Result
CreateConsoleAccount()
{
   auto command = ClientCommand<services::AccountManagerService::CreateConsoleAccount> { internal::getAllocator() };
   command.setParameters();
   return internal::getClient()->sendSyncRequest(command);
}

void
Library::registerAccountManagerServiceSymbols()
{
   RegisterFunctionExportName("CreateConsoleAccount__Q2_2nn3actFv",
                              CreateConsoleAccount);
}

}  // namespace cafe::nn_act

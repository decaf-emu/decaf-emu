#include "nn_act.h"
#include "nn_act_client.h"
#include "nn_act_serverstandardservice.h"

#include "cafe/nn/cafe_nn_ipc_client.h"
#include "nn/act/nn_act_serverstandardservice.h"

#include <chrono>

using namespace nn::act;
using namespace nn::ipc;

namespace cafe::nn_act
{

nn::Result
Cancel()
{
   auto command = ClientCommand<services::ServerStandardService::Cancel> { internal::getAllocator() };
   command.setParameters();
   return internal::getClient()->sendSyncRequest(command);
}

void
Library::registerServerStandardServiceSymbols()
{
   RegisterFunctionExportName("Cancel__Q2_2nn3actFv",
                              Cancel);
}

}  // namespace cafe::nn_act

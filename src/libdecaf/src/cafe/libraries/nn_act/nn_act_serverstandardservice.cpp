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

using services::ServerStandardService;

struct StaticServerData
{
   be2_val<bool> parentalControlCheckEnabled = false;
};

static virt_ptr<StaticServerData> sStaticServerData = nullptr;

nn::Result
AcquireNexServiceToken(virt_ptr<ACTNexAuthenticationResult> result,
                       uint32_t gameId)
{
   auto command = ClientCommand<ServerStandardService::AcquireNexServiceToken> {
         internal::getAllocator()
      };
   command.setParameters(CurrentUserSlot, result, gameId,
                         sStaticServerData->parentalControlCheckEnabled);
   return internal::getClient()->sendSyncRequest(command);
}

nn::Result
Cancel()
{
   auto command = ClientCommand<ServerStandardService::Cancel> {
         internal::getAllocator()
      };
   command.setParameters();
   return internal::getClient()->sendSyncRequest(command);
}

void
EnableParentalControlCheck(bool enable)
{
   sStaticServerData->parentalControlCheckEnabled = enable;
}

bool
IsParentalControlCheckEnabled()
{
   return sStaticServerData->parentalControlCheckEnabled;
}

void
Library::registerServerStandardServiceSymbols()
{
   RegisterFunctionExportName("AcquireNexServiceToken__Q2_2nn3actFP26ACTNexAuthenticationResultUi",
                              AcquireNexServiceToken);
   RegisterFunctionExportName("Cancel__Q2_2nn3actFv",
                              Cancel);
   RegisterFunctionExportName("IsParentalControlCheckEnabled__Q2_2nn3actFv",
                              IsParentalControlCheckEnabled);
   RegisterFunctionExportName("EnableParentalControlCheck__Q2_2nn3actFb",
                              EnableParentalControlCheck);

   RegisterDataInternal(sStaticServerData);
}

}  // namespace cafe::nn_act

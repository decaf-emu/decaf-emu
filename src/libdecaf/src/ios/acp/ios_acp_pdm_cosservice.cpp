#include "ios_acp_pdm_cosservice.h"

#include "ios/nn/ios_nn_ipc_server_command.h"
#include "nn/pdm/nn_pdm_result.h"

#include <chrono>
#include <ctime>

using namespace nn::ipc;
using namespace nn::pdm;

namespace ios::acp::internal
{

static nn::Result
getPlayDiaryMaxLength(CommandHandlerArgs &args)
{
   auto command = ServerCommand<PdmCosService::GetPlayDiaryMaxLength> { args };
   command.WriteResponse(18250);
   return ResultSuccess;
}

static nn::Result
getPlayStatsMaxLength(CommandHandlerArgs &args)
{
   auto command = ServerCommand<PdmCosService::GetPlayStatsMaxLength> { args };
   command.WriteResponse(256);
   return ResultSuccess;
}

nn::Result
PdmCosService::commandHandler(uint32_t unk1,
                              CommandId command,
                              CommandHandlerArgs &args)
{
   switch (command) {
   case GetPlayDiaryMaxLength::command:
      return getPlayDiaryMaxLength(args);
   case GetPlayStatsMaxLength::command:
      return getPlayStatsMaxLength(args);
   default:
      return nn::ResultSuccess;
   }
}

} // namespace ios::acp::internal

#pragma once
#include "ios/nn/ios_nn_ipc_server.h"
#include "nn/pdm/nn_pdm_cosservice.h"

namespace ios::acp::internal
{

struct PdmCosService : ::nn::pdm::services::CosService
{
   static nn::Result
   commandHandler(uint32_t unk1,
                  nn::ipc::CommandId command,
                  nn::ipc::CommandHandlerArgs &args);
};

} // namespace ios::acp::internal

#pragma once
#include "ios/nn/ios_nn_ipc_server.h"
#include "nn/acp/nn_acp_saveservice.h"

namespace ios::acp::internal
{

struct SaveService : ::nn::acp::services::SaveService
{
   static nn::Result
   commandHandler(uint32_t unk1,
                  nn::ipc::CommandId command,
                  nn::ipc::CommandHandlerArgs &args);
};

} // namespace ios::acp::internal

#pragma once
#include "ios/nn/ios_nn_ipc_server.h"
#include "nn/acp/nn_acp_miscservice.h"

namespace ios::acp::internal
{

struct MiscService : ::nn::acp::services::MiscService
{
   static nn::Result
   commandHandler(uint32_t unk1,
                  nn::ipc::CommandId command,
                  nn::ipc::CommandHandlerArgs &args);
};

} // namespace ios::acp

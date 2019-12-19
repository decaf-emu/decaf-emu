#pragma once
#include "ios/nn/ios_nn_ipc_server.h"
#include "nn/ac/nn_ac_service.h"

namespace ios::net::internal
{

struct AcService : ::nn::ac::services::AcService
{
   static nn::Result
   commandHandler(uint32_t unk1,
                  nn::ipc::CommandId command,
                  nn::ipc::CommandHandlerArgs &args);
};

} // namespace ios::net::internal

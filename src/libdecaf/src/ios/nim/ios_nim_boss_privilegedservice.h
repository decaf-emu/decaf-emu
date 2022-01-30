#pragma once
#include "ios/nn/ios_nn_ipc_server.h"
#include "nn/boss/nn_boss_privileged_service.h"

namespace ios::nim::internal
{

struct PrivilegedService : ::nn::boss::services::PrivilegedService
{
   static nn::Result
   commandHandler(uint32_t unk1,
                  nn::ipc::CommandId command,
                  nn::ipc::CommandHandlerArgs &args);
};

} // namespace ios::acp

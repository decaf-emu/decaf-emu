#pragma once
#include "ios/nn/ios_nn_ipc_server.h"
#include "nn/act/nn_act_accountmanagerservice.h"

namespace ios::fpd::internal
{

struct ActAccountManagerService : ::nn::act::services::AccountManagerService
{
   static nn::Result
   commandHandler(uint32_t unk1,
                  nn::ipc::CommandId command,
                  nn::ipc::CommandHandlerArgs &args);
};

} // namespace ios::fpd::internal

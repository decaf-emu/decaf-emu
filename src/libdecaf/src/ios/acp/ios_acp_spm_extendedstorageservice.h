#pragma once
#include "ios/nn/ios_nn_ipc_server.h"
#include "nn/spm/nn_spm_extendedstorageservice.h"

namespace ios::acp::internal
{

struct ExtendedStorageService : ::nn::spm::services::ExtendedStorageService
{
   static nn::Result
   commandHandler(uint32_t unk1,
                  nn::ipc::CommandId command,
                  nn::ipc::CommandHandlerArgs &args);
};

} // namespace ios::acp

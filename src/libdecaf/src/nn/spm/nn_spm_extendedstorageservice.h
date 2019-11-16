#pragma once
#include "nn/ipc/nn_ipc_command.h"
#include "nn/ipc/nn_ipc_service.h"

#include <cstdint>

namespace nn::spm::services
{

// Served from /dev/acp_main
struct ExtendedStorageService : ipc::Service<303>
{
   using SetAutoFatal =
      ipc::Command<ExtendedStorageService, 6>
         ::Parameters<uint8_t>
         ::Response<>;
};

} // namespace nn::spm::services

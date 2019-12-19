#pragma once
#include "nn/ipc/nn_ipc_command.h"
#include "nn/ipc/nn_ipc_managedbuffer.h"
#include "nn/ipc/nn_ipc_service.h"

#include <cstdint>

namespace nn::ac::services
{

struct AcService : ipc::Service<0>
{
   using Initialise =
      ipc::Command<AcService, 80>
         ::Parameters<>
         ::Response<>;

   using Finalise =
      ipc::Command<AcService, 81>
         ::Parameters<>
         ::Response<>;

   using GetAssignedAddress =
      ipc::Command<AcService, 0x610>
         ::Parameters<uint32_t>
         ::Response<uint32_t>;
};

} // namespace nn::ac::services

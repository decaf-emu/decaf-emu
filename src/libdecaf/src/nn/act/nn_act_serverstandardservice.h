#pragma once
#include "nn_act_enum.h"
#include "nn_act_types.h"

#include "nn/ipc/nn_ipc_command.h"
#include "nn/ipc/nn_ipc_service.h"
#include "nn/ipc/nn_ipc_managedbuffer.h"

#include <array>
#include <cstdint>

namespace nn::act::services
{

struct ServerStandardService : ipc::Service<1>
{
   using AcquireNexServiceToken =
      ipc::Command<ServerStandardService, 4>
         ::Parameters<SlotNo, ipc::OutBuffer<NexAuthenticationResult>, uint32_t, bool>;

   using Cancel =
      ipc::Command<ServerStandardService, 100>
         ::Parameters<>;
};

} // namespace nn::act::services

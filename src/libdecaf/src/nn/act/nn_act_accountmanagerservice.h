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

struct AccountManagerService : ipc::Service<2>
{
   using CreateConsoleAccount =
      ipc::Command<AccountManagerService, 1>
         ::Parameters<>;
};

} // namespace nn::act::services

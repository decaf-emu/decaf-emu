#pragma once
#include "nn_act_enum.h"
#include "nn_act_types.h"

#include "nn/ipc/nn_ipc_command.h"
#include "nn/ipc/nn_ipc_service.h"
#include "nn/ipc/nn_ipc_managedbuffer.h"

namespace nn::act::services
{

struct AccountLoaderService : ipc::Service<3>
{
   using LoadConsoleAccount =
      ipc::Command<AccountLoaderService, 1>
         ::Parameters<SlotNo, ACTLoadOption, ipc::InBuffer<const char>, bool>;
};

} // namespace nn::act::services

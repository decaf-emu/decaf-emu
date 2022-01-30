#pragma once
#include "nn_boss_types.h"

#include "nn/ipc/nn_ipc_service.h"

namespace nn::boss::services
{

struct PrivilegedService : ipc::Service<1>
{
   using AddAccount =
      ipc::Command<PrivilegedService, 316>
      ::Parameters<PersistentId>;
};

} // namespace nn::boss::services

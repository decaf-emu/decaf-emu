#pragma once
#include "nn/ipc/nn_ipc_service.h"

namespace nn::boss::services
{

struct ManagementService : ipc::Service<3>
{
};

} // namespace nn::boss::services

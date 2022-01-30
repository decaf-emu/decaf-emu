#pragma once
#include "nn/ipc/nn_ipc_service.h"

namespace nn::boss::services
{

struct BossService : ipc::Service<0>
{
};

} // namespace nn::boss::services

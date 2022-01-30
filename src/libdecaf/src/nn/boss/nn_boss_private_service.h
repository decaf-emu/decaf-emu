#pragma once
#include "nn/ipc/nn_ipc_service.h"

namespace nn::boss::services
{

struct PrivateService : ipc::Service<4>
{
};

} // namespace nn::boss::services

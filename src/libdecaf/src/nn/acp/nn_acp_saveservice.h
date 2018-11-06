#pragma once
#include "nn/ipc/nn_ipc_service.h"

namespace nn::acp::services
{

struct SaveService : ipc::Service<2>
{
};

} // namespace nn::acp::services

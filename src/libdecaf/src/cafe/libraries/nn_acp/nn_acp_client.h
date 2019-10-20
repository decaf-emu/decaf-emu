#pragma once
#include "nn_acp_acpresult.h"

#include "cafe/nn/cafe_nn_ipc_client.h"
#include "cafe/nn/cafe_nn_ipc_bufferallocator.h"

namespace cafe::nn_acp
{

ACPResult
ACPInitialize();

void
ACPFinalize();

namespace internal
{

virt_ptr<nn::ipc::Client>
getClient();

virt_ptr<nn::ipc::BufferAllocator>
getAllocator();

} // namespace internal

} // namespace cafe::nn_acp

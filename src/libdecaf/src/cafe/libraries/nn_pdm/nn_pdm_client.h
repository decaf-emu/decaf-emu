#pragma once
#include "cafe/nn/cafe_nn_ipc_client.h"
#include "cafe/nn/cafe_nn_ipc_bufferallocator.h"

#include "nn/nn_result.h"

namespace cafe::nn_pdm
{

nn::Result
PDMInitialize();

void
PDMFinalize();

namespace internal
{

virt_ptr<nn::ipc::Client>
getClient();

virt_ptr<nn::ipc::BufferAllocator>
getAllocator();

} // namespace internal

} // namespace cafe::nn_pdm

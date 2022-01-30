#pragma once
#include "nn_boss_enum.h"

#include "cafe/nn/cafe_nn_ipc_bufferallocator.h"
#include "cafe/nn/cafe_nn_ipc_client.h"
#include "nn/nn_result.h"

namespace cafe::nn_boss
{

nn::Result
Initialize();

void
Finalize();

bool
IsInitialized();

BossState
GetBossState();


namespace internal
{

virt_ptr<nn::ipc::Client>
getClient();

virt_ptr<nn::ipc::BufferAllocator>
getAllocator();

} // namespace internal

}  // namespace namespace cafe::nn_boss

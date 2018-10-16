#pragma once
#include "nn_boss_enum.h"
#include "nn/nn_result.h"

namespace cafe::nn_boss
{

nn::Result
Initialize();

nn::Result
Finalize();

bool
IsInitialized();

BossState
GetBossState();

}  // namespace namespace cafe::nn_boss

#pragma once
#include "nn_boss_enum.h"
#include "cafe/libraries/nn_result.h"

namespace cafe::nn::boss
{

nn::Result
Initialize();

nn::Result
Finalize();

bool
IsInitialized();

BossState
GetBossState();

}  // namespace namespace cafe::nn::boss

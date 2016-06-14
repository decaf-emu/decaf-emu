#pragma once
#include "modules/nn_result.h"
#include "common/types.h"

namespace nn
{

namespace boss
{

nn::Result
Initialize();

nn::Result
Finalize();

bool
IsInitialized();

}  // namespace boss

}  // namespace nn

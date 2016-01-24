#pragma once
#include "modules/nn_result.h"
#include "types.h"

namespace nn
{

namespace ndm
{

nn::Result
Initialize();

nn::Result
Finalize();

bool
IsInitialized();

}  // namespace ndm

}  // namespace nn

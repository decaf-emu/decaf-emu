#pragma once
#include "nn/nn_result.h"

namespace cafe::nn_ndm
{

nn::Result
Initialize();

nn::Result
Finalize();

bool
IsInitialized();

nn::Result
EnableResumeDaemons();

} // namespace cafe::nn_ndm

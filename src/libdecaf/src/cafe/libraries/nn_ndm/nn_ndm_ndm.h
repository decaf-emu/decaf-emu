#pragma once
#include "cafe/libraries/nn_result.h"

namespace cafe::nn::ndm
{

nn::Result
Initialize();

nn::Result
Finalize();

bool
IsInitialized();

nn::Result
EnableResumeDaemons();

} // namespace cafe::nn::ndm

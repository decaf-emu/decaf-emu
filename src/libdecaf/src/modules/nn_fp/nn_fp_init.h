#pragma once
#include "modules/nn_result.h"

namespace nn
{

namespace fp
{

nn::Result
Initialize();

nn::Result
Finalize();

bool
IsInitialized();

bool
IsOnline();

}  // namespace fp

}  // namespace nn

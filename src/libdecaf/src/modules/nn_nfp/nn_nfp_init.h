#pragma once
#include "modules/nn_result.h"
#include "nn_nfp_enum.h"
#include "common/types.h"

namespace nn
{

namespace nfp
{

nn::Result
Initialize();

nn::Result
Finalize();

State
GetNfpState();

nn::Result
StartDetection();

nn::Result
StopDetection();

}  // namespace nfp

}  // namespace nn

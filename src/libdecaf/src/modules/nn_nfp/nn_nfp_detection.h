#pragma once
#include "modules/nn_result.h"
#include <cstdint>

namespace nn
{

namespace nfp
{

nn::Result
SetActivateEvent(uint32_t);

nn::Result
SetDeactivateEvent(uint32_t);

}  // namespace nfp

}  // namespace nn

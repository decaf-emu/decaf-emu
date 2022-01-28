#pragma once
#include "nn/nn_result.h"

namespace cafe::nn_ec
{

static constexpr nn::Result ResultInvalidArgument {
   nn::Result::MODULE_NN_EC, nn::Result::LEVEL_USAGE, 0x3C80
};

} // namespace cafe::nn_ec

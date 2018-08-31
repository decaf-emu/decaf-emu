#pragma once
#include "cafe/libraries/nn_result.h"

namespace cafe::nn::act
{

static const auto AccountNotFound = nn::Result {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 128128
};

} // namespace cafe::nn::act

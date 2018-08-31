#pragma once
#include "cafe/libraries/nn_result.h"

namespace cafe::nn::boss
{

static const auto Success = Result {
   Result::MODULE_NN_BOSS, Result::LEVEL_SUCCESS, 128
};

static const auto InvalidParameter = Result {
   Result::MODULE_NN_BOSS, Result::LEVEL_USAGE, 14208
};

} // namespace cafe::nn::boss

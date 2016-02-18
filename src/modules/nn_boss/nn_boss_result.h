#pragma once
#include "modules/nn_result.h"

namespace nn
{

namespace boss
{

static const auto Success = nn::Result { nn::Result::MODULE_NN_BOSS, nn::Result::LEVEL_SUCCESS, 128 };
static const auto InvalidParameter = nn::Result { nn::Result::MODULE_NN_BOSS, nn::Result::LEVEL_USAGE, 14208 };

} // namespace boss

} // namespace nn
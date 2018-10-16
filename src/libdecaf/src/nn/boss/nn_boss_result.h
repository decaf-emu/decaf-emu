#pragma once
#include "nn/nn_result.h"

namespace nn::boss
{

static constexpr Result ResultSuccess {
   Result::MODULE_NN_BOSS, Result::LEVEL_SUCCESS, 128
};

static constexpr Result ResultInvalidParameter {
   Result::MODULE_NN_BOSS, Result::LEVEL_USAGE, 14208
};

} // namespace nn::boss

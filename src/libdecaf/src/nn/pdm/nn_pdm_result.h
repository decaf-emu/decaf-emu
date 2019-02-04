#pragma once
#include "nn/nn_result.h"

namespace nn::pdm
{

static constexpr Result ResultSuccess {
   nn::Result::MODULE_NN_PDM, nn::Result::LEVEL_SUCCESS, 128
};

} // namespace nn::pdm

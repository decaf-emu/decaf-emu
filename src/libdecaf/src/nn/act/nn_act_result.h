#pragma once
#include "nn/nn_result.h"

namespace nn::act
{

static constexpr Result ResultAccountNotFound {
   nn::Result::MODULE_NN_ACT, nn::Result::LEVEL_STATUS, 128128
};

} // namespace nn::act

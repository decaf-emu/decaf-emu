#pragma once
#include "nn/nn_result.h"

namespace nn::acp
{

static constexpr Result ResultSuccess {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_SUCCESS, 128
};

static constexpr Result ResultXmlNodeNotFound {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_STATUS, 64896
};

static constexpr Result ResultXmlRootNodeNotFound {
   nn::Result::MODULE_NN_ACP, nn::Result::LEVEL_FATAL, 25984
};

} // namespace nn::acp

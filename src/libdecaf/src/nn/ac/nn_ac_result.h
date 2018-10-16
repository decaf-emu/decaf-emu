#pragma once
#include "nn/nn_result.h"

namespace nn::ac
{

static constexpr Result ResultSuccess {
   nn::Result::MODULE_NN_AC, nn::Result::LEVEL_SUCCESS, 128
};

static constexpr Result ResultInvalidArgument {
   nn::Result::MODULE_NN_AC, nn::Result::LEVEL_USAGE, 51584
};

static constexpr Result ResultLibraryNotInitialiased {
   nn::Result::MODULE_NN_AC, nn::Result::LEVEL_USAGE, 52224
};

static constexpr Result ResultConnectFailed {
   nn::Result::MODULE_NN_AC, nn::Result::LEVEL_STATUS, 65408
};

} // namespace nn::ac

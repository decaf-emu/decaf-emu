#pragma once
#include "nn/nn_result.h"

namespace nn::ac
{

static constexpr Result ResultSuccess {
   nn::Result::MODULE_NN_AC, nn::Result::LEVEL_SUCCESS, 128
};

static constexpr Result ResultInvalidArgument {
   nn::Result::MODULE_NN_AC, nn::Result::LEVEL_USAGE, 0xC980
};

static constexpr Result ResultLibraryNotInitialiased {
   nn::Result::MODULE_NN_AC, nn::Result::LEVEL_USAGE, 0xCC00
};

static constexpr Result ResultConnectFailed {
   nn::Result::MODULE_NN_AC, nn::Result::LEVEL_STATUS, 0xFF80
};

} // namespace nn::ac

#pragma once
#include "nn/nn_result.h"

namespace nn::nfp
{

static constexpr Result ResultSuccess {
   Result::MODULE_NN_NFP, Result::LEVEL_SUCCESS, 128
};

static constexpr Result ResultInvalidPointer {
   Result::MODULE_NN_NFP, Result::LEVEL_USAGE, 14208
};

static constexpr Result ResultIsBusy {
   Result::MODULE_NN_NFP, Result::LEVEL_STATUS, 38528
};

static constexpr Result ResultInvalidState {
   Result::MODULE_NN_NFP, Result::LEVEL_STATUS, 256000
};

} // namespace nn::nfp

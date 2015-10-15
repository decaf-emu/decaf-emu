#pragma once
#include "modules/nn_result.h"

namespace nn
{

namespace nfp
{

static const auto InvalidPointer = nn::Result { nn::Result::MODULE_NN_NFP, nn::Result::LEVEL_USAGE, 14208 };
static const auto IsBusy = nn::Result { nn::Result::MODULE_NN_NFP, nn::Result::LEVEL_STATUS, 38528 };
static const auto InvalidState = nn::Result { nn::Result::MODULE_NN_NFP, nn::Result::LEVEL_STATUS, 256000 };

} // namespace nfp

} // namespace nn
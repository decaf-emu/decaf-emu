#pragma once
#include "cafe/libraries/nn_result.h"

namespace cafe::nn::nfp
{

static const auto InvalidPointer = Result {
   Result::MODULE_NN_NFP, Result::LEVEL_USAGE, 14208
};

static const auto IsBusy = Result {
   Result::MODULE_NN_NFP, Result::LEVEL_STATUS, 38528
};

static const auto InvalidState = Result {
   Result::MODULE_NN_NFP, Result::LEVEL_STATUS, 256000
};

} // namespace cafe::nn::nfp
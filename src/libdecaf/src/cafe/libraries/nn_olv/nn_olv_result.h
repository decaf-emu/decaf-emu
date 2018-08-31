#pragma once
#include "cafe/libraries/nn_result.h"

namespace cafe::nn::olv
{

static const auto Success =
   nn::Result { nn::Result::MODULE_NN_OLV, nn::Result::LEVEL_SUCCESS, 0x80 };

static const auto InvalidSize =
   nn::Result { nn::Result::MODULE_NN_OLV, nn::Result::LEVEL_USAGE, 0x6580 };

static const auto InvalidPointer =
   nn::Result { nn::Result::MODULE_NN_OLV, nn::Result::LEVEL_USAGE, 0x6600 };

static const auto NoData =
   nn::Result { nn::Result::MODULE_NN_OLV, nn::Result::LEVEL_USAGE, 0x6800 };

} // namespace cafe::nn::olv

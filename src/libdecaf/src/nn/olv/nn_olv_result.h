#pragma once
#include "nn/nn_result.h"

namespace nn::olv
{

static constexpr Result ResultSuccess {
   nn::Result::MODULE_NN_OLV, nn::Result::LEVEL_SUCCESS, 0x80
};

static constexpr Result ResultInvalidSize {
   nn::Result::MODULE_NN_OLV, nn::Result::LEVEL_USAGE, 0x6580
};

static constexpr Result ResultInvalidPointer {
   nn::Result::MODULE_NN_OLV, nn::Result::LEVEL_USAGE, 0x6600
};

static constexpr Result ResultNotOnline {
   nn::Result::MODULE_NN_OLV, nn::Result::LEVEL_USAGE, 0x6780
};

static constexpr Result ResultNoData {
   nn::Result::MODULE_NN_OLV, nn::Result::LEVEL_USAGE, 0x6800
};

} // namespace nn::olv

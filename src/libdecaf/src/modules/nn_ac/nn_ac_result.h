#pragma once
#include "modules/nn_result.h"

namespace nn
{

namespace ac
{

static const auto InvalidArgument = nn::Result { nn::Result::MODULE_NN_AC, nn::Result::LEVEL_USAGE, 51584 };
static const auto LibraryNotInitialiased = nn::Result { nn::Result::MODULE_NN_AC, nn::Result::LEVEL_USAGE, 52224 };

} // namespace ac

} // namespace nn
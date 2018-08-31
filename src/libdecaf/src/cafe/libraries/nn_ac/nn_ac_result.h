#pragma once
#include "cafe/libraries/nn_result.h"

namespace cafe::nn::ac
{

constexpr auto InvalidArgument = nn::Result {
   nn::Result::MODULE_NN_AC, nn::Result::LEVEL_USAGE, 51584
};

constexpr auto LibraryNotInitialiased = nn::Result {
   nn::Result::MODULE_NN_AC, nn::Result::LEVEL_USAGE, 52224
};

constexpr auto ConnectFailed = nn::Result {
   nn::Result::MODULE_NN_AC, nn::Result::LEVEL_STATUS, 65408
};

} // namespace cafe::nn::ac

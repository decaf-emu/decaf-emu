#pragma once
#include "nn/nn_result.h"

namespace nn::dbg
{

const char *
GetDescriptionString(nn::Result result);

const char *
GetLevelString(nn::Result result);

const char *
GetModuleString(nn::Result result);

const char *
GetSummaryString(nn::Result result);

} // namespace nn::dbg

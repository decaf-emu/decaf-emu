#pragma once
#include "types.h"
#include "modules/nn_result.h"
#include "common/be_val.h"

namespace nn
{

namespace acp
{

nn::Result
ACPCheckApplicationDeviceEmulation(be_val<BOOL> *isEmulated);

}  // namespace acp

}  // namespace nn

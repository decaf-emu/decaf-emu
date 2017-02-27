#pragma once
#include "modules/nn_result.h"

#include <common/be_val.h>
#include <common/cbool.h>

namespace nn
{

namespace acp
{

nn::Result
ACPCheckApplicationDeviceEmulation(be_val<BOOL> *isEmulated);

}  // namespace acp

}  // namespace nn

#pragma once
#include "cafe/libraries/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn::acp
{

nn::Result
ACPCheckApplicationDeviceEmulation(virt_ptr<BOOL> outValue);

}  // namespace cafe::nn::acp

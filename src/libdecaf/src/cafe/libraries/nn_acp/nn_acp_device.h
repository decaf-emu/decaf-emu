#pragma once
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_acp
{

nn::Result
ACPCheckApplicationDeviceEmulation(virt_ptr<BOOL> outValue);

}  // namespace cafe::nn_acp

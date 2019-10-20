#pragma once
#include "nn_acp_acpresult.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_acp
{

ACPResult
ACPCheckApplicationDeviceEmulation(virt_ptr<BOOL> outValue);

}  // namespace cafe::nn_acp

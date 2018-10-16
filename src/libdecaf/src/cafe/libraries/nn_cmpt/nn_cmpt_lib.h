#pragma once
#include "nn_cmpt_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::nn_cmpt
{

CMPTError
CMPTGetDataSize(virt_ptr<uint32_t> outDataSize);

} // namespace cafe::nn_cmpt

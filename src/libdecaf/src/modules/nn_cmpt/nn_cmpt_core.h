#pragma once
#include "modules/nn_result.h"

#include <cstdint>
#include <common/be_val.h>

namespace nn
{

namespace cmpt
{

using CMPTError = int32_t;

CMPTError
CMPTGetDataSize(be_val<uint32_t> *size);

}  // namespace ac

}  // namespace nn

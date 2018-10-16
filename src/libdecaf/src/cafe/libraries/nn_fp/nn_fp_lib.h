#pragma once
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_fp
{

nn::Result
Initialize();

nn::Result
Finalize();

bool
IsInitialized();

bool
IsOnline();

nn::Result
GetFriendList(virt_ptr<void> list,
              virt_ptr<uint32_t> outLength,
              uint32_t index,
              uint32_t listSize);

} // namespace cafe::nn_fp

#pragma once
#include "be_val.h"
#include "types.h"
#include "modules/nn_result.h"

namespace nn
{

namespace fp
{

nn::Result
Initialize();

nn::Result
Finalize();

bool
IsInitialized();

nn::Result
GetFriendList(void *list, be_val<uint32_t> *length, uint32_t index, uint32_t listSize);

}  // namespace fp

}  // namespace nn

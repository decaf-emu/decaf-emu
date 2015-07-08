#pragma once
#include "systemtypes.h"

namespace nn
{

namespace fp
{

enum class Result : uint32_t
{
   OK = 0,
   Error = 0x80000000,
};

Result
Initialize();

Result
Finalize();

bool
IsInitialized();

Result
GetFriendList(void *list, be_val<uint32_t> *length, uint32_t index, uint32_t listSize);

}  // namespace fp

}  // namespace nn

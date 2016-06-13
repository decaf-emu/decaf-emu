#pragma once
#include "common/types.h"
#include "modules/nn_result.h"

namespace nn
{

namespace ac
{

enum class Status : int32_t
{
   Error = -1,
   OK = 0
};

nn::Result
Initialize();

void
Finalize();

nn::Result
Connect();

nn::Result
IsApplicationConnected(bool *connected);

nn::Result
GetConnectStatus(Status *status);

nn::Result
GetLastErrorCode(uint32_t *error);

}  // namespace ac

}  // namespace nn

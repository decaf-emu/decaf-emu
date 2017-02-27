#pragma once
#include "modules/nn_result.h"

#include <cstdint>

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

nn::Result
GetStatus(nn::ac::Status *status);

}  // namespace ac

}  // namespace nn

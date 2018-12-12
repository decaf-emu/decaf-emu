#pragma once
#include "nn_ac_enum.h"
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_ac
{

using ConfigId = int32_t;

nn::Result
Initialize();

void
Finalize();

nn::Result
Connect();

nn::Result
ConnectAsync();

nn::Result
IsApplicationConnected(virt_ptr<bool> connected);

nn::Result
GetConnectStatus(virt_ptr<Status> outStatus);

nn::Result
GetLastErrorCode(virt_ptr<int32_t> outError);

nn::Result
GetStatus(virt_ptr<Status> outStatus);

nn::Result
GetStartupId(virt_ptr<ConfigId> outStartupId);

}  // namespace cafe::nn_ac

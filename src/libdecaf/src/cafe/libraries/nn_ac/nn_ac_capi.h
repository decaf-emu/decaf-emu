#pragma once
#include "nn_ac_enum.h"
#include "cafe/libraries/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn::ac
{

nn::Result
ACInitialize();

void
ACFinalize();

nn::Result
ACConnect();

nn::Result
ACConnectAsync();

nn::Result
ACIsApplicationConnected(virt_ptr<BOOL> connected);

nn::Result
ACGetConnectStatus(virt_ptr<Status> outStatus);

nn::Result
ACGetLastErrorCode(virt_ptr<int32_t> outError);

nn::Result
ACGetStatus(virt_ptr<Status> outStatus);

}  // namespace cafe::nn::ac

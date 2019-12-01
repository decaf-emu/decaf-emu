#pragma once
#include "nn/nn_result.h"
#include "nn/act/nn_act_types.h"

namespace cafe::nn_act
{

using ACTNexAuthenticationResult = nn::act::NexAuthenticationResult;

nn::Result
AcquireNexServiceToken(virt_ptr<ACTNexAuthenticationResult> result,
                       uint32_t unk);

nn::Result
Cancel();

bool
IsParentalControlCheckEnabled();

void
EnableParentalControlCheck(bool enable);

}  // namespace cafe::nn_act

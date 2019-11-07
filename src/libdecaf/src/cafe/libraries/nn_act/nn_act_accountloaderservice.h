#pragma once
#include "nn/nn_result.h"
#include "nn/act/nn_act_enum.h"
#include "nn/act/nn_act_types.h"

namespace cafe::nn_act
{

nn::Result
LoadConsoleAccount(nn::act::SlotNo slot,
                   nn::act::ACTLoadOption option,
                   virt_ptr<const char> arg3,
                   bool arg4);

}  // namespace cafe::nn_act

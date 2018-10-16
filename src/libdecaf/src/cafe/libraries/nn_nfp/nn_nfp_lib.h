#pragma once
#include "nn_nfp_enum.h"

#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_nfp
{

struct AmiiboSettingsArgs
{
   UNKNOWN(0x5D);
};
CHECK_SIZE(AmiiboSettingsArgs, 0x5D);

nn::Result
Initialize();

nn::Result
Finalize();

nn::Result
GetAmiiboSettingsArgs(virt_ptr<AmiiboSettingsArgs> args);

State
GetNfpState();

nn::Result
SetActivateEvent(uint32_t a1);

nn::Result
SetDeactivateEvent(uint32_t a1);

nn::Result
StartDetection();

nn::Result
StopDetection();

}  // namespace cafe::nn_nfp

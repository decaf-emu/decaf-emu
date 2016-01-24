#pragma once
#include "modules/nn_result.h"
#include "types.h"
#include "utils/structsize.h"

namespace nn
{

namespace nfp
{

struct AmiiboSettingsArgs
{
   UNKNOWN(0x5D);
};
CHECK_SIZE(AmiiboSettingsArgs, 0x5D);

nn::Result
GetAmiiboSettingsArgs(AmiiboSettingsArgs *args);

}  // namespace nfp

}  // namespace nn

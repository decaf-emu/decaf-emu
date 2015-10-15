#pragma once
#include "be_val.h"
#include "structsize.h"
#include "types.h"
#include "modules/nn_result.h"

namespace nn
{

namespace nfp
{

struct AmiiboSettingsArgs
{
   UNKNOWN(0x5D);
};

nn::Result
Initialize();

nn::Result
Finalize();

nn::Result
GetAmiiboSettingsArgs(AmiiboSettingsArgs *args);

}  // namespace fp

}  // namespace nn

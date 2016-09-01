#include "nn_nfp.h"
#include "nn_nfp_settings.h"

namespace nn
{

namespace nfp
{

nn::Result
GetAmiiboSettingsArgs(AmiiboSettingsArgs *args)
{
   decaf_warn_stub();

   std::memset(args, 0, sizeof(AmiiboSettingsArgs));
   return nn::Result::Success;
}

void
Module::registerSettingsFunctions()
{
   RegisterKernelFunctionName("GetAmiiboSettingsArgs__Q2_2nn3nfpFPQ3_2nn3nfp18AmiiboSettingsArgs", nn::nfp::GetAmiiboSettingsArgs);
}

} // namespace nfp

} // namespace nn

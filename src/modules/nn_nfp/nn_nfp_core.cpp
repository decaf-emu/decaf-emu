#include "nn_nfp.h"
#include "nn_nfp_core.h"

namespace nn
{

namespace nfp
{

nn::Result
Initialize()
{
   return nn::Result::Success;
}

nn::Result
Finalize()
{
   return nn::Result::Success;
}

nn::Result
GetAmiiboSettingsArgs(AmiiboSettingsArgs *args)
{
   memset(args, 0, sizeof(AmiiboSettingsArgs));
   return nn::Result::Success;
}

nn::Result
SetActivateEvent(uint32_t)
{
   // TODO: nn::nfp::SetActivateEvent
   return nn::Result::Success;
}

nn::Result
SetDeactivateEvent(uint32_t)
{
   // TODO: nn::nfp::SetDeactivateEvent
   return nn::Result::Success;
}

} // namespace nfp

} // namespace nn

void
NN_nfp::registerCoreFunctions()
{
   RegisterKernelFunctionName("Initialize__Q2_2nn3nfpFv", nn::nfp::Initialize);
   RegisterKernelFunctionName("Finalize__Q2_2nn3nfpFv", nn::nfp::Finalize);
   RegisterKernelFunctionName("GetAmiiboSettingsArgs__Q2_2nn3nfpFPQ3_2nn3nfp18AmiiboSettingsArgs", nn::nfp::GetAmiiboSettingsArgs);
   RegisterKernelFunctionName("SetActivateEvent__Q2_2nn3nfpFP7OSEvent", nn::nfp::SetActivateEvent);
   RegisterKernelFunctionName("SetDeactivateEvent__Q2_2nn3nfpFP7OSEvent", nn::nfp::SetDeactivateEvent);
}

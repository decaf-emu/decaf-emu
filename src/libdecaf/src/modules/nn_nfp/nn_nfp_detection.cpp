#include "nn_nfp.h"
#include "nn_nfp_detection.h"

namespace nn
{

namespace nfp
{

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

void
Module::registerDetectionFunctions()
{
   RegisterKernelFunctionName("SetActivateEvent__Q2_2nn3nfpFP7OSEvent", nn::nfp::SetActivateEvent);
   RegisterKernelFunctionName("SetDeactivateEvent__Q2_2nn3nfpFP7OSEvent", nn::nfp::SetDeactivateEvent);
}

} // namespace nfp

} // namespace nn

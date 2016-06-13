#include "nn_acp.h"
#include "nn_acp_title.h"

namespace nn
{

namespace acp
{

nn::Result
ACPCheckApplicationDeviceEmulation(be_val<BOOL> *isEmulated)
{
   // -0x1F4 is error code
   *isEmulated = FALSE;
   return nn::Result::Success;
}

void
Module::registerDeviceFunctions()
{
   RegisterKernelFunctionName("ACPCheckApplicationDeviceEmulation", nn::acp::ACPCheckApplicationDeviceEmulation);
}

}  // namespace acp

}  // namespace nn


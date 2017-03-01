#include "nn_acp.h"
#include "nn_acp_device.h"

namespace nn
{

namespace acp
{

nn::Result
ACPCheckApplicationDeviceEmulation(be_val<BOOL> *isEmulated)
{
   decaf_warn_stub();

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


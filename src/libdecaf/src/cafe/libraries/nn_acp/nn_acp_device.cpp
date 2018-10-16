#include "nn_acp.h"
#include "nn_acp_device.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "nn/acp/nn_acp_result.h"

using namespace nn::acp;

namespace cafe::nn_acp
{

nn::Result
ACPCheckApplicationDeviceEmulation(virt_ptr<BOOL> outValue)
{
   decaf_warn_stub();
   *outValue = FALSE;
   return ResultSuccess;
}

void
Library::registerDeviceSymbols()
{
   RegisterFunctionExportName("ACPCheckApplicationDeviceEmulation",
                              ACPCheckApplicationDeviceEmulation);
}

}  // namespace cafe::nn_acp

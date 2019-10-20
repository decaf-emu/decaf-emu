#include "nn_acp.h"
#include "nn_acp_device.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nn_acp
{

ACPResult
ACPCheckApplicationDeviceEmulation(virt_ptr<BOOL> outValue)
{
   decaf_warn_stub();
   *outValue = FALSE;
   return ACPResult::Success;
}

void
Library::registerDeviceSymbols()
{
   RegisterFunctionExportName("ACPCheckApplicationDeviceEmulation",
                              ACPCheckApplicationDeviceEmulation);
}

}  // namespace cafe::nn_acp

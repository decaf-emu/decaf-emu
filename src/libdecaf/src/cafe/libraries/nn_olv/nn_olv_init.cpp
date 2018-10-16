#include "nn_olv.h"
#include "nn_olv_init.h"
#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nn_olv
{

static bool
gInitialised = false;

nn::Result
Initialize(virt_ptr<InitializeParam> initParam)
{
   decaf_warn_stub();
   gInitialised = true;
   return nn::ResultSuccess;
}

nn::Result
Initialize(virt_ptr<MainAppParam> mainAppParam,
           virt_ptr<InitializeParam> initParam)
{
   decaf_warn_stub();
   gInitialised = true;
   return nn::ResultSuccess;
}

nn::Result
Finalize()
{
   decaf_warn_stub();
   gInitialised = false;
   return nn::ResultSuccess;
}

bool
IsInitialized()
{
   decaf_warn_stub();
   return gInitialised;
}

void
Library::registerInitSymbols()
{
   RegisterFunctionExportName("Initialize__Q2_2nn3olvFPCQ3_2nn3olv15InitializeParam",
                              static_cast<nn::Result (*)(virt_ptr<InitializeParam>)>(Initialize));
   RegisterFunctionExportName("Initialize__Q2_2nn3olvFPQ3_2nn3olv12MainAppParamPCQ3_2nn3olv15InitializeParam",
                              static_cast<nn::Result (*)(virt_ptr<MainAppParam>, virt_ptr<InitializeParam>)>(Initialize));
   RegisterFunctionExportName("Finalize__Q2_2nn4bossFv", Finalize);
   RegisterFunctionExportName("IsInitialized__Q2_2nn3olvFv", IsInitialized);
}

}  // namespace cafe::nn_olv

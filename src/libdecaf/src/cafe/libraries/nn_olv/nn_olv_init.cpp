#include "nn_olv.h"
#include "nn_olv_init.h"
#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nn::olv
{

static bool
gInitialised = false;

nn::Result
Initialize(virt_ptr<InitializeParam> initParam)
{
   decaf_warn_stub();
   gInitialised = true;
   return nn::Result::Success;
}

nn::Result
Initialize(virt_ptr<MainAppParam> mainAppParam,
           virt_ptr<InitializeParam> initParam)
{
   decaf_warn_stub();
   gInitialised = true;
   return nn::Result::Success;
}

nn::Result
Finalize()
{
   decaf_warn_stub();
   gInitialised = false;
   return nn::Result::Success;
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
                              static_cast<nn::Result (*)(virt_ptr<InitializeParam>)>(nn::olv::Initialize));
   RegisterFunctionExportName("Initialize__Q2_2nn3olvFPQ3_2nn3olv12MainAppParamPCQ3_2nn3olv15InitializeParam",
                              static_cast<nn::Result (*)(virt_ptr<MainAppParam>, virt_ptr<InitializeParam>)>(nn::olv::Initialize));
   RegisterFunctionExportName("Finalize__Q2_2nn4bossFv", nn::olv::Finalize);
   RegisterFunctionExportName("IsInitialized__Q2_2nn3olvFv", nn::olv::IsInitialized);
}

}  // namespace cafe::nn::olv

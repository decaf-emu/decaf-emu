#include "nn_olv.h"
#include "nn_olv_init.h"

namespace nn
{

namespace olv
{

static bool
gInitialised = false;

nn::Result
Initialize(InitializeParam *initParam)
{
   decaf_warn_stub();

   gInitialised = true;
   return nn::Result::Success;
}

nn::Result
Initialize(MainAppParam *mainAppParam, InitializeParam *initParam)
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
Module::registerInitFunctions()
{
   RegisterKernelFunctionName("Initialize__Q2_2nn3olvFPCQ3_2nn3olv15InitializeParam",
                              static_cast<nn::Result (*)(InitializeParam *)>(nn::olv::Initialize));
   RegisterKernelFunctionName("Initialize__Q2_2nn3olvFPQ3_2nn3olv12MainAppParamPCQ3_2nn3olv15InitializeParam",
                              static_cast<nn::Result (*)(MainAppParam *,InitializeParam *)>(nn::olv::Initialize));
   RegisterKernelFunctionName("Finalize__Q2_2nn4bossFv", nn::olv::Finalize);
   RegisterKernelFunctionName("IsInitialized__Q2_2nn3olvFv", nn::olv::IsInitialized);
}

}  // namespace boss

}  // namespace nn

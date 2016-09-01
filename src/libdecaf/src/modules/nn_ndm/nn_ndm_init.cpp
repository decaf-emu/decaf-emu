#include "nn_ndm.h"
#include "nn_ndm_init.h"

namespace nn
{

namespace ndm
{

static bool
gInitialised = false;

nn::Result
Initialize()
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

nn::Result
EnableResumeDaemons()
{
   decaf_warn_stub();

   return nn::Result::Success;
}

void
Module::registerInitFunctions()
{
   RegisterKernelFunctionName("Initialize__Q2_2nn3ndmFv", nn::ndm::Initialize);
   RegisterKernelFunctionName("Finalize__Q2_2nn3ndmFv", nn::ndm::Finalize);
   RegisterKernelFunctionName("IsInitialized__Q2_2nn3ndmFv", nn::ndm::IsInitialized);
   RegisterKernelFunctionName("EnableResumeDaemons__Q2_2nn3ndmFv", nn::ndm::EnableResumeDaemons);
}

}  // namespace ndm

}  // namespace nn

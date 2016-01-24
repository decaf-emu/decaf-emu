#include "nn_ndm.h"
#include "nn_ndm_init.h"

namespace nn
{

namespace ndm
{

static bool
gInitialised = false;


static bool
gInitialised = false;

nn::Result
Initialize()
{
   gInitialised = true;
   return nn::Result::Success;
}

nn::Result
Finalize()
{
   gInitialised = false;
   return nn::Result::Success;
}

bool
IsInitialized()
{
   return gInitialised;
}

void
Module::registerInitFunctions()
{
   RegisterKernelFunctionName("Initialize__Q2_2nn3ndmFv", nn::ndm::Initialize);
   RegisterKernelFunctionName("Finalize__Q2_2nn3ndmFv", nn::ndm::Finalize);
   RegisterKernelFunctionName("IsInitialized__Q2_2nn3ndmFv", nn::ndm::IsInitialized);
}

}  // namespace ndm

}  // namespace nn

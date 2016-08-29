#include "nn_fp.h"
#include "nn_fp_init.h"

namespace nn
{

namespace fp
{

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

bool
IsOnline()
{
   return false;
}

void
Module::registerInitFunctions()
{
   RegisterKernelFunctionName("Initialize__Q2_2nn2fpFv", nn::fp::Initialize);
   RegisterKernelFunctionName("Finalize__Q2_2nn2fpFv", nn::fp::Finalize);
   RegisterKernelFunctionName("IsInitialized__Q2_2nn2fpFv", nn::fp::IsInitialized);
   RegisterKernelFunctionName("IsOnline__Q2_2nn2fpFv", nn::fp::IsOnline);
}

} // namespace fp

} // namespace nn

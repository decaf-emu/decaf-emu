#include "nn_nfp.h"
#include "nn_nfp_init.h"

namespace nn
{

namespace nfp
{

nn::Result
Initialize()
{
   return nn::Result::Success;
}

nn::Result
Finalize()
{
   return nn::Result::Success;
}

void
Module::registerInitFunctions()
{
   RegisterKernelFunctionName("Initialize__Q2_2nn3nfpFv", nn::nfp::Initialize);
   RegisterKernelFunctionName("Finalize__Q2_2nn3nfpFv", nn::nfp::Finalize);
}

} // namespace nfp

} // namespace nn

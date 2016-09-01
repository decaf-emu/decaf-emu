#include "nn_nfp.h"
#include "nn_nfp_init.h"

namespace nn
{

namespace nfp
{

static State
sState = State::Uninitialised;

nn::Result
Initialize()
{
   decaf_warn_stub();

   sState = State::Initialised;
   return nn::Result::Success;
}

nn::Result
Finalize()
{
   decaf_warn_stub();

   sState = State::Uninitialised;
   return nn::Result::Success;
}

State
GetNfpState()
{
   decaf_warn_stub();

   return sState;
}

nn::Result
StartDetection()
{
   decaf_warn_stub();

   sState = State::Detecting;
   return nn::Result::Success;
}

nn::Result
StopDetection()
{
   decaf_warn_stub();

   sState = State::Initialised;
   return nn::Result::Success;
}

void
Module::registerInitFunctions()
{
   RegisterKernelFunctionName("Initialize__Q2_2nn3nfpFv", nn::nfp::Initialize);
   RegisterKernelFunctionName("Finalize__Q2_2nn3nfpFv", nn::nfp::Finalize);
   RegisterKernelFunctionName("GetNfpState__Q2_2nn3nfpFv", nn::nfp::GetNfpState);
   RegisterKernelFunctionName("StartDetection__Q2_2nn3nfpFv", nn::nfp::StartDetection);
   RegisterKernelFunctionName("StopDetection__Q2_2nn3nfpFv", nn::nfp::StopDetection);
}

} // namespace nfp

} // namespace nn

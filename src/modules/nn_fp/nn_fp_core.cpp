#include "nn_fp.h"
#include "nn_fp_core.h"

namespace nn
{

namespace fp
{

static bool
gInitialised = false;

Result
Initialize()
{
   gInitialised = true;
   return Result::OK;
}

Result
Finalize()
{
   gInitialised = false;
   return Result::OK;
}

bool
IsInitialized()
{
   return gInitialised;
}

Result
GetFriendList(void *list, be_val<uint32_t> *length, uint32_t index, uint32_t listSize)
{
   *length = 0;
   return Result::OK;
}

} // namespace fp

} // namespace nn

void
NNFp::registerCoreFunctions()
{
   RegisterKernelFunctionName("Initialize__Q2_2nn2fpFv", nn::fp::Initialize);
   RegisterKernelFunctionName("Finalize__Q2_2nn2fpFv", nn::fp::Finalize);
   RegisterKernelFunctionName("IsInitialized__Q2_2nn2fpFv", nn::fp::IsInitialized);
   RegisterKernelFunctionName("GetFriendList__Q2_2nn2fpFPUiT1UiT3", nn::fp::GetFriendList);
}

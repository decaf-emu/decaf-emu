#include "nn_ac.h"
#include "nn_ac_core.h"

namespace nn
{

namespace ac
{

Result
Initialize()
{
   return Result::OK;
}

void
Finalize()
{
}

Result
Connect()
{
   return Result::Error;
}

Result
IsApplicationConnected(bool *connected)
{
   *connected = false;
   return Result::OK;
}

Result
GetConnectStatus(Status *status)
{
   *status = Status::Error;
   return Result::OK;
}

Result
GetLastErrorCode(uint32_t *error)
{
   *error = 123;
   return Result::OK;
}

} // namespace ac

} // namespace nn

void
NNAc::registerCoreFunctions()
{
   RegisterKernelFunctionName("Initialize__Q2_2nn2acFv", nn::ac::Initialize);
   RegisterKernelFunctionName("Finalize__Q2_2nn2acFv", nn::ac::Finalize);
   RegisterKernelFunctionName("Connect__Q2_2nn2acFv", nn::ac::Connect);
   RegisterKernelFunctionName("IsApplicationConnected__Q2_2nn2acFPb", nn::ac::IsApplicationConnected);
   RegisterKernelFunctionName("GetConnectStatus__Q2_2nn2acFPQ3_2nn2ac6Status", nn::ac::GetConnectStatus);
   RegisterKernelFunctionName("GetLastErrorCode__Q2_2nn2acFPUi", nn::ac::GetLastErrorCode);
}

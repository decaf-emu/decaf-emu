#include "nn_ac.h"
#include "nn_ac_core.h"
#include "nn_ac_result.h"

namespace nn
{

namespace ac
{

nn::Result
Initialize()
{
   decaf_warn_stub();

   return nn::Result::Success;
}

void
Finalize()
{
   decaf_warn_stub();
}

nn::Result
Connect()
{
   decaf_warn_stub();

   return ac::ConnectFailed;
}

nn::Result
ConnectAsync()
{
   decaf_warn_stub();

   return nn::Result::Success;
}

nn::Result
IsApplicationConnected(bool *connected)
{
   decaf_warn_stub();

   *connected = false;
   return nn::Result::Success;
}

nn::Result
GetConnectStatus(Status *status)
{
   decaf_warn_stub();

   *status = Status::Error;
   return nn::Result::Success;
}

nn::Result
GetLastErrorCode(uint32_t *error)
{
   decaf_warn_stub();

   *error = 123;
   return nn::Result::Success;
}

nn::Result
GetStatus(nn::ac::Status *status)
{
   decaf_warn_stub();

   return GetConnectStatus(status);
}

void
Module::registerCoreFunctions()
{
   RegisterKernelFunctionName("Initialize__Q2_2nn2acFv", nn::ac::Initialize);
   RegisterKernelFunctionName("Finalize__Q2_2nn2acFv", nn::ac::Finalize);
   RegisterKernelFunctionName("Connect__Q2_2nn2acFv", nn::ac::Connect);
   RegisterKernelFunctionName("ConnectAsync__Q2_2nn2acFv", nn::ac::ConnectAsync);
   RegisterKernelFunctionName("IsApplicationConnected__Q2_2nn2acFPb", nn::ac::IsApplicationConnected);
   RegisterKernelFunctionName("GetConnectStatus__Q2_2nn2acFPQ3_2nn2ac6Status", nn::ac::GetConnectStatus);
   RegisterKernelFunctionName("GetLastErrorCode__Q2_2nn2acFPUi", nn::ac::GetLastErrorCode);
   RegisterKernelFunctionName("GetStatus__Q2_2nn2acFPQ3_2nn2ac6Status", nn::ac::GetStatus);
}

} // namespace ac

} // namespace nn

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
   return nn::Result::Success;
}

void
Finalize()
{
}

nn::Result
Connect()
{
   // TODO: Find a better error to return? :D
   return ac::LibraryNotInitialiased;
}

nn::Result
IsApplicationConnected(bool *connected)
{
   *connected = false;
   return nn::Result::Success;
}

nn::Result
GetConnectStatus(Status *status)
{
   *status = Status::Error;
   return nn::Result::Success;
}

nn::Result
GetLastErrorCode(uint32_t *error)
{
   *error = 123;
   return nn::Result::Success;
}

} // namespace ac

} // namespace nn

void
NN_ac::registerCoreFunctions()
{
   RegisterKernelFunctionName("Initialize__Q2_2nn2acFv", nn::ac::Initialize);
   RegisterKernelFunctionName("Finalize__Q2_2nn2acFv", nn::ac::Finalize);
   RegisterKernelFunctionName("Connect__Q2_2nn2acFv", nn::ac::Connect);
   RegisterKernelFunctionName("IsApplicationConnected__Q2_2nn2acFPb", nn::ac::IsApplicationConnected);
   RegisterKernelFunctionName("GetConnectStatus__Q2_2nn2acFPQ3_2nn2ac6Status", nn::ac::GetConnectStatus);
   RegisterKernelFunctionName("GetLastErrorCode__Q2_2nn2acFPUi", nn::ac::GetLastErrorCode);
}

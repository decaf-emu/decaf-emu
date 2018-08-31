#include "nn_ac.h"
#include "nn_ac_lib.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nn::ac
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
   return nn::Result::Success;
}

nn::Result
ConnectAsync()
{
   decaf_warn_stub();
   return nn::Result::Success;
}

nn::Result
IsApplicationConnected(virt_ptr<bool> connected)
{
   decaf_warn_stub();
   *connected = false;
   return nn::Result::Success;
}

nn::Result
GetConnectStatus(virt_ptr<Status> outStatus)
{
   decaf_warn_stub();
   *outStatus = Status::Error;
   return nn::Result::Success;
}

nn::Result
GetLastErrorCode(virt_ptr<int32_t> outError)
{
   decaf_warn_stub();
   *outError = -1;
   return nn::Result::Success;
}

nn::Result
GetStatus(virt_ptr<Status> outStatus)
{
   decaf_warn_stub();
   *outStatus = Status::Error;
   return nn::Result::Success;
}

void
Library::registerLibFunctions()
{
   RegisterFunctionExportName("Initialize__Q2_2nn2acFv",
                              Initialize);
   RegisterFunctionExportName("Finalize__Q2_2nn2acFv",
                              Finalize);
   RegisterFunctionExportName("Connect__Q2_2nn2acFv",
                              Connect);
   RegisterFunctionExportName("ConnectAsync__Q2_2nn2acFv",
                              ConnectAsync);
   RegisterFunctionExportName("IsApplicationConnected__Q2_2nn2acFPb",
                              IsApplicationConnected);
   RegisterFunctionExportName("GetConnectStatus__Q2_2nn2acFPQ3_2nn2ac6Status",
                              GetConnectStatus);
   RegisterFunctionExportName("GetLastErrorCode__Q2_2nn2acFPUi",
                              GetLastErrorCode);
   RegisterFunctionExportName("GetStatus__Q2_2nn2acFPQ3_2nn2ac6Status",
                              GetStatus);
}


}  // namespace cafe::nn::ac

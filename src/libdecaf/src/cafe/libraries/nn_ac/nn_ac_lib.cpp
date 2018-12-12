#include "nn_ac.h"
#include "nn_ac_lib.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "nn/ac/nn_ac_result.h"

using namespace nn::ac;

namespace cafe::nn_ac
{

nn::Result
Initialize()
{
   decaf_warn_stub();
   return ResultSuccess;
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
   return ResultSuccess;
}

nn::Result
ConnectAsync()
{
   decaf_warn_stub();
   return ResultSuccess;
}

nn::Result
IsApplicationConnected(virt_ptr<bool> connected)
{
   decaf_warn_stub();
   *connected = false;
   return ResultSuccess;
}

nn::Result
GetConnectStatus(virt_ptr<Status> outStatus)
{
   decaf_warn_stub();
   *outStatus = Status::Error;
   return ResultSuccess;
}

nn::Result
GetLastErrorCode(virt_ptr<int32_t> outError)
{
   decaf_warn_stub();
   *outError = -1;
   return ResultSuccess;
}

nn::Result
GetStatus(virt_ptr<Status> outStatus)
{
   decaf_warn_stub();
   *outStatus = Status::Error;
   return ResultSuccess;
}

nn::Result
GetStartupId(virt_ptr<ConfigId> outStartupId)
{
   decaf_warn_stub();
   *outStartupId = 0;
   return ResultSuccess;
}

nn::Result
ReadConfig(ConfigId id,
           virt_ptr<Config> config)
{
   decaf_warn_stub();
   std::memset(config.get(), 0, sizeof(Config));
   return ResultSuccess;
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
   RegisterFunctionExportName("GetStartupId__Q2_2nn2acFPQ3_2nn2ac11ConfigIdNum",
                              GetStartupId);
   RegisterFunctionExportName("ReadConfig__Q2_2nn2acFQ3_2nn2ac11ConfigIdNumP16netconf_profile_",
                              ReadConfig);
}

}  // namespace cafe::nn_ac

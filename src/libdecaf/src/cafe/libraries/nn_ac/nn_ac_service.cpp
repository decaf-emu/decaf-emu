#include "nn_ac.h"
#include "nn_ac_client.h"
#include "nn_ac_service.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "nn/ac/nn_ac_result.h"
#include "nn/ac/nn_ac_service.h"
#include "nn/ipc/nn_ipc_command.h"

using namespace nn::ac;
using namespace nn::ipc;

namespace cafe::nn_ac
{

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
GetAssignedAddress(virt_ptr<uint32_t> outAddress)
{
   if (!internal::getClient()->isInitialised()) {
      return ResultLibraryNotInitialiased;
   }

   if (!outAddress) {
      return ResultInvalidArgument;
   }

   auto command = ClientCommand<services::AcService::GetAssignedAddress> { internal::getAllocator() };
   command.setParameters(0);

   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.ok()) {
      auto address = uint32_t{ 0 };
      result = command.readResponse(address);
      if (result.ok()) {
         *outAddress = address;
      }
   }

   return result;
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
Library::registerServiceSymbols()
{
   RegisterFunctionExportName("Connect__Q2_2nn2acFv",
                              Connect);
   RegisterFunctionExportName("ConnectAsync__Q2_2nn2acFv",
                              ConnectAsync);
   RegisterFunctionExportName("IsApplicationConnected__Q2_2nn2acFPb",
                              IsApplicationConnected);
   RegisterFunctionExportName("GetAssignedAddress__Q2_2nn2acFPUl",
                              GetAssignedAddress);
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

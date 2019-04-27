#include "nn_ac.h"
#include "nn_ac_capi.h"
#include "nn_ac_lib.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/cafe_stackobject.h"

namespace cafe::nn_ac
{

nn::Result
ACInitialize()
{
   return Initialize();
}

void
ACFinalize()
{
   return Finalize();
}

nn::Result
ACConnect()
{
   return Connect();
}

nn::Result
ACConnectAsync()
{
   return ConnectAsync();
}

nn::Result
ACIsApplicationConnected(virt_ptr<BOOL> connected)
{
   auto isConnected = StackObject<bool> { };
   *isConnected = *connected ? true : false;
   auto result = IsApplicationConnected(isConnected);
   *connected = *isConnected ? TRUE : FALSE;
   return result;
}

nn::Result
ACGetConnectStatus(virt_ptr<Status> outStatus)
{
   return GetConnectStatus(outStatus);
}

nn::Result
ACGetLastErrorCode(virt_ptr<int32_t> outError)
{
   return GetLastErrorCode(outError);
}

nn::Result
ACGetStatus(virt_ptr<Status> outStatus)
{
   return GetStatus(outStatus);
}

nn::Result
ACGetStartupId(virt_ptr<ConfigId> outStartupId)
{
   return GetStartupId(outStartupId);
}

nn::Result
ACReadConfig(ConfigId id,
             virt_ptr<Config> config)
{
   return ReadConfig(id, config);
}

void
Library::registerCApiFunctions()
{
   RegisterFunctionExport(ACInitialize);
   RegisterFunctionExport(ACFinalize);
   RegisterFunctionExport(ACConnect);
   RegisterFunctionExport(ACConnectAsync);
   RegisterFunctionExport(ACIsApplicationConnected);
   RegisterFunctionExport(ACGetConnectStatus);
   RegisterFunctionExport(ACGetLastErrorCode);
   RegisterFunctionExport(ACGetStatus);
   RegisterFunctionExport(ACGetStartupId);
   RegisterFunctionExport(ACReadConfig);
}

}  // namespace cafe::nn_ac

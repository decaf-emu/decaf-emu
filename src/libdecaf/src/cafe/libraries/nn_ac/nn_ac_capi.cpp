#include "nn_ac.h"
#include "nn_ac_capi.h"
#include "nn_ac_lib.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/cafe_stackobject.h"

namespace cafe::nn::ac
{

nn::Result
ACInitialize()
{
   return nn::ac::Initialize();
}

void
ACFinalize()
{
   return nn::ac::Finalize();
}

nn::Result
ACConnect()
{
   return nn::ac::Connect();
}

nn::Result
ACConnectAsync()
{
   return nn::ac::ConnectAsync();
}

nn::Result
ACIsApplicationConnected(virt_ptr<BOOL> connected)
{
   StackObject<bool> isConnected;
   *isConnected = *connected ? true : false;
   auto result = nn::ac::IsApplicationConnected(isConnected);
   *connected = *isConnected ? TRUE : FALSE;
   return result;
}

nn::Result
ACGetConnectStatus(virt_ptr<Status> outStatus)
{
   return nn::ac::GetConnectStatus(outStatus);
}

nn::Result
ACGetLastErrorCode(virt_ptr<int32_t> outError)
{
   return nn::ac::GetLastErrorCode(outError);
}

nn::Result
ACGetStatus(virt_ptr<Status> outStatus)
{
   return nn::ac::GetStatus(outStatus);
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
}

}  // namespace cafe::nn::ac

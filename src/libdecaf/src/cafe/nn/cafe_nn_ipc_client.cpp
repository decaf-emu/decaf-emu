#include "cafe_nn_ipc_client.h"

#include "cafe/libraries/coreinit/coreinit_ios.h"
#include "nn/nn_result.h"

using namespace cafe::coreinit;

namespace nn::ipc
{

Result
Client::initialise(virt_ptr<const char> device)
{
   auto error = IOS_Open(device, IOSOpenMode::None);
   if (error < 0) {
      return ios::convertError(error);
   }

   mHandle = static_cast<IOSHandle>(error);
   return ios::ResultOK;
}

Result
Client::close()
{
   return ios::convertError(IOS_Close(mHandle));
}

Result
Client::sendSyncRequest(const detail::ClientCommandData &command)
{
   auto error = IOS_Ioctlv(mHandle, 0,
                           command.numVecIn,
                           command.numVecOut,
                           command.vecsBuffer);

   if (error < 0) {
      return ios::convertError(error);
   }

   return ResultSuccess;
}

} // namespace nn::ipc

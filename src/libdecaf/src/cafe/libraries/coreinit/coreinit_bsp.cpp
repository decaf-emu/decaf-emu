#include "coreinit.h"
#include "coreinit_bsp.h"
#include "coreinit_ios.h"
#include "cafe/cafe_stackobject.h"
#include "ios/bsp/ios_bsp_bsp_request.h"
#include "ios/bsp/ios_bsp_bsp_response.h"

namespace cafe::coreinit
{

using ios::bsp::BSPRequest;
using ios::bsp::BSPResponse;
using ios::bsp::BSPResponseGetHardwareVersion;

struct BSPIpcBuffer
{
   be2_struct<BSPRequest> request;
   UNKNOWN(0x80 - 0x48);
   be2_struct<BSPResponse> response;
};

struct StaticBspData
{
   be2_val<IOSHandle> bspHandle;
};

static virt_ptr<StaticBspData>
sBspData = nullptr;

namespace internal
{

static BSPError
prepareIpcBuffer(std::size_t responseSize,
                 virt_ptr<BSPIpcBuffer> buffer);

}

BSPError
bspInitializeShimInterface()
{
   auto result = IOS_Open(make_stack_string("/dev/bsp"), ios::OpenMode::None);

   if (IOS_FAILED(result)) {
      return BSPError::IosError;
   }

   sBspData->bspHandle = static_cast<IOSHandle>(result);
   return BSPError::OK;
}

BSPError
bspGetHardwareVersion(virt_ptr<BSPHardwareVersion> version)
{
   StackObject<BSPIpcBuffer> buffer;
   auto error = internal::prepareIpcBuffer(sizeof(BSPResponseGetHardwareVersion),
                                           buffer);
   if (error) {
      return error;
   }

   auto result = IOS_Ioctl(sBspData->bspHandle,
                           2,
                           virt_addrof(buffer->request),
                           sizeof(BSPRequest),
                           virt_addrof(buffer->response.getHardwareVersion),
                           sizeof(BSPResponseGetHardwareVersion));

   if (IOS_FAILED(result)) {
      return BSPError::IosError;
   }

   *version = buffer->response.getHardwareVersion.hardwareVersion;
   return  BSPError::OK;
}

namespace internal
{

static BSPError
prepareIpcBuffer(std::size_t responseSize,
                 virt_ptr<BSPIpcBuffer> buffer)
{
   if (responseSize > sizeof(ios::bsp::BSPResponse)) {
      return BSPError::ResponseTooLarge;
   }

   std::memset(virt_addrof(buffer->request).getRawPointer(), 0,
               sizeof(BSPRequest));
   return BSPError::OK;
}

BSPError
bspShutdownShimInterface()
{
   auto result = IOS_Close(sBspData->bspHandle);

   if (IOS_FAILED(result)) {
      return BSPError::IosError;
   }

   sBspData->bspHandle = IOSError::Invalid;
   return BSPError::OK;
}

} // namespace internal

void
Library::registerBspSymbols()
{
   RegisterFunctionExport(bspInitializeShimInterface);
   RegisterFunctionExport(bspGetHardwareVersion);

   RegisterDataInternal(sBspData);
}

} // namespace cafe::coreinit

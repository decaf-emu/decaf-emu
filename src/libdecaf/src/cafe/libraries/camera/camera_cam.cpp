#include "camera.h"
#include "camera_cam.h"
#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::camera
{

CAMHandle
CAMInit(uint32_t id,
        virt_ptr<CAMInitInfo> info,
        virt_ptr<CAMError> outError)
{
   decaf_warn_stub();
   *outError = CAMError::OK;
   return id;
}

void
CAMExit(CAMHandle handle)
{
   decaf_warn_stub();
}

int32_t
CAMOpen(CAMHandle handle)
{
   decaf_warn_stub();
   return CAMError::OK;
}

int32_t
CAMClose(CAMHandle handle)
{
   decaf_warn_stub();
   return CAMError::OK;
}

int32_t
CAMGetMemReq(virt_ptr<CAMMemoryInfo> info)
{
   decaf_warn_stub();

   if (!info) {
      return -1;
   }

   auto bufferSize = info->width * info->height * 10;
   auto extraSize = 2 * 0x14 + 0x5400;
   return bufferSize + extraSize;
}

void
Library::registerCamSymbols()
{
   RegisterFunctionExport(CAMInit);
   RegisterFunctionExport(CAMExit);
   RegisterFunctionExport(CAMOpen);
   RegisterFunctionExport(CAMClose);
   RegisterFunctionExport(CAMGetMemReq);
}

} // namespace cafe::camera

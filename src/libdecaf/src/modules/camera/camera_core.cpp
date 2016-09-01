#include "camera.h"
#include "camera_core.h"

namespace camera
{

CAMHandle
CAMInit(uint32_t id,
        CAMInitInfo *info,
        be_val<CAMError> *error)
{
   decaf_warn_stub();

   *error = CAMError::OK;
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
CAMGetMemReq(CAMMemoryInfo *info)
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
Module::registerCoreFunctions()
{
   RegisterKernelFunction(CAMInit);
   RegisterKernelFunction(CAMExit);
   RegisterKernelFunction(CAMOpen);
   RegisterKernelFunction(CAMClose);
   RegisterKernelFunction(CAMGetMemReq);
}

} // namespace camera

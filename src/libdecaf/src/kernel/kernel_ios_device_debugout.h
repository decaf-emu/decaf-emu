#pragma once
#include "kernel_ios_device.h"

#include <common/log.h>

namespace kernel
{

class DebugOutDevice : public IOSDevice
{
public:
   virtual ~DebugOutDevice() = default;

   virtual IOSError
   open(IOSOpenMode mode);

   virtual IOSError
   close();

   virtual IOSError
   read(void *buffer,
        size_t length);

   virtual IOSError
   write(void *buffer,
         size_t length);

   virtual IOSError
   ioctl(uint32_t request,
         void *inBuf,
         size_t inLen,
         void *outBuf,
         size_t outLen);

   virtual IOSError
   ioctlv(uint32_t request,
          size_t vecIn,
          size_t vecOut,
          IOSVec *vec);

private:
};

} // namespace kernel

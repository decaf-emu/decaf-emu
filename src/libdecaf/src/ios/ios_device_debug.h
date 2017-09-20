#pragma once
#include "ios_device.h"

namespace ios
{

class DebugDevice : public IOSDevice
{
public:
   virtual ~DebugDevice() = default;

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

} // namespace ios

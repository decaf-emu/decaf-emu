#pragma once
#include "ios_device.h"

namespace ios
{

class DebugDevice : public IOSDevice
{
public:
   virtual ~DebugDevice() = default;

   virtual Error
   open(OpenMode mode);

   virtual Error
   close();

   virtual Error
   read(void *buffer,
         size_t length);

   virtual Error
   write(void *buffer,
         size_t length);

   virtual Error
   ioctl(uint32_t request,
         void *inBuf,
         size_t inLen,
         void *outBuf,
         size_t outLen);

   virtual Error
   ioctlv(uint32_t request,
          size_t vecIn,
          size_t vecOut,
          IoctlVec *vec);

private:
};

} // namespace ios

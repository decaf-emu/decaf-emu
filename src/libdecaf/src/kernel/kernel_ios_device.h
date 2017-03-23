#pragma once
#include "kernel_ios.h"

#include <cstdint>

namespace kernel
{

class IOSDevice
{
public:
   virtual ~IOSDevice() = default;

   virtual IOSError
   open(IOSOpenMode mode)
   {
      return IOSError::OK;
   }

   virtual IOSError
   close()
   {
      return IOSError::OK;
   }

   virtual IOSError
   read(void *buffer,
        size_t length)
   {
      return IOSError::FailInternal;
   }

   virtual IOSError
   write(void *buffer,
         size_t length)
   {
      return IOSError::FailInternal;
   }

   virtual IOSError
   ioctl(uint32_t request,
         void *inBuf,
         size_t inLen,
         void *outBuf,
         size_t outLen)
   {
      return IOSError::FailInternal;
   }

   virtual IOSError
   ioctlv(uint32_t request,
          size_t vecIn,
          size_t vecOut,
          IOSVec *vec)
   {
      return IOSError::FailInternal;
   }

   IOSHandle
   handle() const
   {
      return mHandle;
   }

   void
   setHandle(IOSHandle handle)
   {
      mHandle = handle;
   }

private:
   IOSHandle mHandle;
};

} // namespace kernel

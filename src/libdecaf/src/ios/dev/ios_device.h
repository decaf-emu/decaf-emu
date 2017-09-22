#pragma once
#include "ios/ios_enum.h"
#include "ios/ios_ipc.h"

#include <cstdint>

namespace ios
{

class IOSDevice
{
public:
   virtual ~IOSDevice() = default;

   virtual Error
   open(OpenMode mode)
   {
      return Error::OK;
   }

   virtual Error
   close()
   {
      return Error::OK;
   }

   virtual Error
   read(void *buffer,
        size_t length)
   {
      return Error::FailInternal;
   }

   virtual Error
   write(void *buffer,
         size_t length)
   {
      return Error::FailInternal;
   }

   virtual Error
   ioctl(uint32_t request,
         void *inBuf,
         size_t inLen,
         void *outBuf,
         size_t outLen)
   {
      return Error::FailInternal;
   }

   virtual Error
   ioctlv(uint32_t request,
          size_t vecIn,
          size_t vecOut,
          IoctlVec *vec)
   {
      return Error::FailInternal;
   }

   int32_t
   handle() const
   {
      return mHandle;
   }

   const std::string &
   name() const
   {
      return mName;
   }

   void
   setHandle(int32_t handle)
   {
      mHandle = handle;
   }

   void
   setName(const std::string &name)
   {
      mName = name;
   }

protected:
   int32_t mHandle;
   std::string mName;
};

} // namespace ios

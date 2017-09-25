#pragma once
#include "ios_enum.h"
#include "ios_ipc.h"
#include "kernel/ios_kernel_resourcemanager.h"
#include <libcpu/be2_struct.h>

namespace ios
{

class Device
{
public:
   virtual ~Device() = 0;

   virtual Error
   read(phys_ptr<kernel::ResourceRequest> resourceRequest,
        phys_ptr<void> buffer,
        uint32_t length) = 0;

   virtual Error
   write(phys_ptr<kernel::ResourceRequest> resourceRequest,
         phys_ptr<const void> buffer,
         uint32_t length) = 0;

   virtual Error
   seek(phys_ptr<kernel::ResourceRequest> resourceRequest,
        uint32_t offset,
        SeekOrigin origin) = 0;

   virtual Error
   ioctl(phys_ptr<kernel::ResourceRequest> resourceRequest,
         uint32_t ioctlRequest,
         phys_ptr<void> inputBuffer,
         uint32_t inputLength,
         phys_ptr<void> outputBuffer,
         uint32_t outputLength) = 0;

   virtual Error
   ioctlv(phys_ptr<kernel::ResourceRequest> resourceRequest,
          uint32_t ioctlRequest,
          uint32_t numVecsIn,
          uint32_t numVecsOut,
          phys_ptr<IoctlVec> vecs) = 0;
};

} // namespace ios

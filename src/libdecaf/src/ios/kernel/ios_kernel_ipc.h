#pragma once
#include "ios_kernel_messagequeue.h"
#include "ios_kernel_resourcemanager.h"
#include "ios/ios_ipc.h"

namespace ios::kernel
{

Error
IOS_Open(std::string_view device,
         OpenMode mode);

Error
IOS_Close(ResourceHandleID handle);

Error
IOS_Read(ResourceHandleID handle,
         phys_ptr<void> buffer,
         uint32_t length);

Error
IOS_Write(ResourceHandleID handle,
          phys_ptr<const void> buffer,
          uint32_t length);
Error
IOS_Seek(ResourceHandleID handle,
         uint32_t offset,
         uint32_t origin);

Error
IOS_Ioctl(ResourceHandleID handle,
          uint32_t request,
          phys_ptr<const void> inputBuffer,
          uint32_t inputBufferLength,
          phys_ptr<void> outputBuffer,
          uint32_t outputBufferLength);

Error
IOS_Ioctlv(ResourceHandleID handle,
           uint32_t request,
           uint32_t numVecIn,
           uint32_t numVecOut,
           phys_ptr<IoctlVec> vecs);

Error
IOS_Resume(ResourceHandleID handle,
           uint32_t unkArg0,
           uint32_t unkArg1);

Error
IOS_Suspend(ResourceHandleID handle,
            uint32_t unkArg0,
            uint32_t unkArg1);

Error
IOS_SvcMsg(ResourceHandleID handle,
           uint32_t unkArg0,
           uint32_t unkArg1,
           uint32_t unkArg2,
           uint32_t unkArg3);

} // namespace ios::kernel

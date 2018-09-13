#pragma once
#include "cafe_kernel_ipckdriver.h"
#include "cafe_kernel_processid.h"

#include "ios/ios_ipc.h"

namespace cafe::kernel
{

ios::Handle
getMcpHandle();

namespace internal
{

ios::Error
IOS_OpenAsync(RamPartitionId clientProcessId,
              virt_ptr<const char> device,
              ios::OpenMode mode,
              IPCKDriverHostAsyncCallbackFn asyncCallback,
              virt_ptr<void> asyncCallbackData);

ios::Error
IOS_Open(RamPartitionId clientProcessId,
         virt_ptr<const char> device,
         ios::OpenMode mode);

ios::Error
IOS_CloseAsync(RamPartitionId clientProcessId,
               ios::Handle handle,
               IPCKDriverHostAsyncCallbackFn asyncCallback,
               virt_ptr<void> asyncCallbackData,
               uint32_t unkArg0);

ios::Error
IOS_Close(RamPartitionId clientProcessId,
          ios::Handle handle,
          uint32_t unkArg0);

ios::Error
IOS_IoctlAsync(RamPartitionId clientProcessId,
               RamPartitionId loaderProcessId,
               ios::Handle handle,
               uint32_t request,
               virt_ptr<void> inBuf,
               uint32_t inLen,
               virt_ptr<void> outBuf,
               uint32_t outLen,
               IPCKDriverHostAsyncCallbackFn asyncCallback,
               virt_ptr<void> asyncCallbackData);

ios::Error
IOS_Ioctl(RamPartitionId clientProcessId,
          RamPartitionId loaderProcessId,
          ios::Handle handle,
          uint32_t request,
          virt_ptr<void> inBuf,
          uint32_t inLen,
          virt_ptr<void> outBuf,
          uint32_t outLen);

virt_ptr<void>
ipcAllocBuffer(uint32_t size,
               int32_t *outError = nullptr);

void
ipcFreeBuffer(virt_ptr<void> buffer);

void
initialiseIpc();

void
initialiseStaticIpcData();

} // namespace internal

} // namespace cafe::kernel::internal

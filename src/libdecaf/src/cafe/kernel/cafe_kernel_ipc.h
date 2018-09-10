#pragma once
#include "cafe_kernel_ipckdriver.h"
#include "cafe_kernel_processid.h"

#include "ios/ios_ipc.h"

namespace cafe::kernel::internal
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

virt_ptr<void>
ipcAllocBuffer(uint32_t size,
               int32_t *outError = nullptr);

void
ipcFreeBuffer(virt_ptr<void> buffer);

void
initialiseIpc();

void
initialiseStaticIpcData();

} // namespace cafe::kernel::internal

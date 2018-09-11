#include "cafe_kernel.h"
#include "cafe_kernel_ipc.h"
#include "cafe_kernel_ipckdriver.h"
#include "cafe_kernel_heap.h"
#include "cafe_kernel_lock.h"
#include "cafe_kernel_mmu.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/cafe_tinyheap.h"
#include "ios/ios_ipc.h"

#include <libcpu/cpu.h>
#include <mutex>

namespace cafe::kernel::internal
{

constexpr auto IpcBufferSize = 0x4000u;
constexpr auto IpcBufferAlign = 0x40u;

struct StaticIpcData
{
   be2_virt_ptr<TinyHeap> ipcHeap;
   be2_array<std::byte, IpcBufferSize> ipcHeapBuffer;
   be2_val<int32_t> mcpHandle;
   be2_val<int32_t> ppcAppHandle;
   be2_val<int32_t> cblHandle;
};

struct SynchronousCallback
{
   std::atomic<bool> replyReceived = false;
   ios::Error error = ios::Error::InvalidArg;
   virt_ptr<void> buffer = nullptr;
};

static virt_ptr<StaticIpcData>
sIpcData;

static std::mutex
sIpcHeapMutex;

static void
ipcInitialiseHeap()
{
   sIpcData->ipcHeap = virt_cast<TinyHeap *>(virt_addrof(sIpcData->ipcHeapBuffer));
   TinyHeap_Setup(sIpcData->ipcHeap,
                  0x430,
                  virt_addrof(sIpcData->ipcHeapBuffer) + 0x430,
                  sIpcData->ipcHeapBuffer.size() - 0x430);
}

virt_ptr<void>
ipcAllocBuffer(uint32_t size,
               int32_t *outError)
{
   auto lock = std::unique_lock { sIpcHeapMutex };
   auto allocPtr = virt_ptr<void> { nullptr };
   auto error = TinyHeap_Alloc(sIpcData->ipcHeap,
                               align_up(size, IpcBufferAlign),
                               IpcBufferAlign,
                               &allocPtr);
   if (outError) {
      *outError = static_cast<int32_t>(error);
   }

   return allocPtr;
}

void
ipcFreeBuffer(virt_ptr<void> buffer)
{
   auto lock = std::unique_lock { sIpcHeapMutex };
   TinyHeap_Free(sIpcData->ipcHeap, buffer);
}

static void
synchronousCallback(ios::Error error,
                    virt_ptr<void> context)
{
   auto synchronousReply = virt_cast<SynchronousCallback *>(context);
   synchronousReply->error = error;
   synchronousReply->replyReceived = true;

   if (synchronousReply->buffer) {
      ipcFreeBuffer(synchronousReply->buffer);
   }
}

static ios::Error
waitSynchronousReply(virt_ptr<SynchronousCallback> synchronousReply,
                     std::chrono::microseconds timeout,
                     uint32_t unk)
{
   auto waitUntil = std::chrono::steady_clock::now() + timeout;
   auto error = ios::Error::Timeout;

   while (!synchronousReply->replyReceived) {
      cpu::this_core::waitNextInterrupt(waitUntil);

      if (std::chrono::steady_clock::now() >= waitUntil) {
         break;
      }
   }

   if (synchronousReply->replyReceived) {
      error = synchronousReply->error;
      synchronousReply->replyReceived = false;
      synchronousReply->buffer = nullptr;
   }

   return error;
}

ios::Error
IOS_OpenAsync(RamPartitionId clientProcessId,
              virt_ptr<const char> device,
              ios::OpenMode mode,
              IPCKDriverHostAsyncCallbackFn asyncCallback,
              virt_ptr<void> asyncCallbackData)
{
   virt_ptr<IPCKDriverRequestBlock> requestBlock;
   auto driver = ipckDriverGetInstance();
   auto error = ipckDriverAllocateRequestBlock(clientProcessId,
                                               RamPartitionId::Invalid,
                                               driver,
                                               &requestBlock,
                                               0,
                                               ios::Command::Open,
                                               asyncCallback,
                                               asyncCallbackData);
   if (error < ios::Error::OK) {
      return error;
   }

   requestBlock->request->request.args.open.name = effectiveToPhysical(device);
   requestBlock->request->request.args.open.nameLen =
      static_cast<uint32_t>(strlen(device.getRawPointer()));

   requestBlock->request->request.args.open.mode = mode;
   requestBlock->request->request.args.open.caps = 0ull;

   error = ipckDriverSubmitRequest(driver, requestBlock);
   if (error < ios::Error::OK) {
      ipckDriverFreeRequestBlock(driver, requestBlock);
   }

   return error;
}

ios::Error
IOS_Open(RamPartitionId clientProcessId,
         virt_ptr<const char> device,
         ios::OpenMode mode)
{
   StackObject<SynchronousCallback> synchronousReply;
   auto error = IOS_OpenAsync(clientProcessId,
                              device,
                              mode,
                              &synchronousCallback,
                              synchronousReply);
   if (error < ios::Error::OK) {
      return error;
   }

   return waitSynchronousReply(synchronousReply,
                               std::chrono::milliseconds { 35 },
                               6);
}

ios::Error
IOS_CloseAsync(RamPartitionId clientProcessId,
               ios::Handle handle,
               IPCKDriverHostAsyncCallbackFn asyncCallback,
               virt_ptr<void> asyncCallbackData,
               uint32_t unkArg0)
{
   virt_ptr<IPCKDriverRequestBlock> requestBlock;
   auto driver = ipckDriverGetInstance();
   auto error = ipckDriverAllocateRequestBlock(clientProcessId,
                                               RamPartitionId::Invalid,
                                               driver,
                                               &requestBlock,
                                               handle,
                                               ios::Command::Open,
                                               asyncCallback,
                                               asyncCallbackData);
   if (error < ios::Error::OK) {
      return error;
   }

   requestBlock->request->request.args.close.unkArg0 = unkArg0;

   error = ipckDriverSubmitRequest(driver, requestBlock);
   if (error < ios::Error::OK) {
      ipckDriverFreeRequestBlock(driver, requestBlock);
   }

   return error;
}

ios::Error
IOS_Close(RamPartitionId clientProcessId,
          ios::Handle handle,
          uint32_t unkArg0)
{
   StackObject<SynchronousCallback> synchronousReply;
   auto error = IOS_CloseAsync(clientProcessId,
                               handle,
                               &synchronousCallback,
                               synchronousReply,
                               unkArg0);
   if (error < ios::Error::OK) {
      return error;
   }

   return waitSynchronousReply(synchronousReply, std::chrono::milliseconds { 35 }, 6);
}

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
               virt_ptr<void> asyncCallbackData)
{
   virt_ptr<IPCKDriverRequestBlock> requestBlock;
   auto driver = ipckDriverGetInstance();
   auto error = ipckDriverAllocateRequestBlock(clientProcessId,
                                               loaderProcessId,
                                               driver,
                                               &requestBlock,
                                               handle,
                                               ios::Command::Ioctl,
                                               asyncCallback,
                                               asyncCallbackData);
   if (error < ios::Error::OK) {
      return error;
   }


   auto &ioctl = requestBlock->request->request.args.ioctl;
   ioctl.request = request;
   ioctl.inputLength = inLen;
   ioctl.outputLength = outLen;

   if (inBuf) {
      ioctl.inputBuffer = effectiveToPhysical(inBuf);
   } else {
      ioctl.inputBuffer = nullptr;
   }

   if (outBuf) {
      ioctl.outputBuffer = effectiveToPhysical(outBuf);
   } else {
      ioctl.outputBuffer = nullptr;
   }

   error = ipckDriverSubmitRequest(driver, requestBlock);
   if (error < ios::Error::OK) {
      ipckDriverFreeRequestBlock(driver, requestBlock);
   }

   return error;
}

ios::Error
IOS_Ioctl(RamPartitionId clientProcessId,
          RamPartitionId loaderProcessId,
          ios::Handle handle,
          uint32_t request,
          virt_ptr<void> inBuf,
          uint32_t inLen,
          virt_ptr<void> outBuf,
          uint32_t outLen)
{
   StackObject<SynchronousCallback> synchronousReply;
   auto error = IOS_IoctlAsync(clientProcessId,
                               loaderProcessId,
                               handle,
                               request,
                               inBuf, inLen,
                               outBuf, outLen,
                               &synchronousCallback,
                               synchronousReply);
   if (error < ios::Error::OK) {
      return error;
   }

   return waitSynchronousReply(synchronousReply, std::chrono::seconds { 35 }, 6);
}

void
initialiseIpc()
{
   ipcInitialiseHeap();
   auto nameBuffer = virt_cast<char *>(ipcAllocBuffer(0x20));

   std::strncpy(nameBuffer.getRawPointer(), "/dev/mcp", 0x20);
   sIpcData->mcpHandle = IOS_Open(RamPartitionId::Kernel,
                                  nameBuffer,
                                  ios::OpenMode::None);

   std::strncpy(nameBuffer.getRawPointer(), "/dev/ppc_app", 0x20);
   sIpcData->ppcAppHandle = IOS_Open(RamPartitionId::Kernel,
                                     nameBuffer,
                                     ios::OpenMode::None);

   std::strncpy(nameBuffer.getRawPointer(), "/dev/cbl", 0x20);
   sIpcData->cblHandle = IOS_Open(RamPartitionId::Kernel,
                                  nameBuffer,
                                  ios::OpenMode::None);

   ipcFreeBuffer(nameBuffer);
}

void
initialiseStaticIpcData()
{
   sIpcData = allocStaticData<StaticIpcData>();
}

} // namespace cafe::kernel::internal

namespace cafe::kernel
{

ios::Handle
getMcpHandle()
{
   return internal::sIpcData->mcpHandle;
}

} // namespace cafe::kernel

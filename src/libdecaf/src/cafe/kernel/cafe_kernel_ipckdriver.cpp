#include "cafe_kernel_ipckdriver.h"
#include "cafe_kernel_interrupts.h"
#include "cafe_kernel_heap.h"
#include "cafe_kernel_process.h"

#include "cafe/libraries/coreinit/coreinit_ipcdriver.h"
#include "cafe/libraries/coreinit/coreinit_scheduler.h"
#include "ios/kernel/ios_kernel_ipc_thread.h"

#include <condition_variable>
#include <libcpu/cpu.h>
#include <mutex>
#include <queue>

namespace cafe::kernel
{

struct StaticIpckDriverData
{
   be2_array<internal::IPCKDriver, 3> drivers;
   be2_array<IPCKDriverRequest, internal::IPCKRequestsPerCore * 3> requestBuffer;
};

static virt_ptr<StaticIpckDriverData>
sIpckDriverData = nullptr;

static std::mutex
sIpcMutex;

static std::vector<phys_ptr<ios::IpcRequest>>
sPendingResponses[3];


namespace internal
{

ios::Error
submitUserRequest(virt_ptr<IPCKDriver> driver,
                  virt_ptr<IPCKDriverRequest> request);

ios::Error
submitLoaderRequest(virt_ptr<IPCKDriver> driver,
                    virt_ptr<IPCKDriverRequest> request);

} // namespace internal


/**
 * Open the current core's IPCKDriver for the current userland process.
 */
ios::Error
ipckDriverUserOpen(uint32_t numReplies,
                   virt_ptr<IPCKDriverReplyQueue> replyQueue,
                   InterruptHandlerFn handler)
{
   auto driver = internal::ipckDriverGetInstance();
   if (!driver) {
      return ios::Error::FailInternal;
   }

   auto pidx = static_cast<uint32_t>(getCurrentRampid());
   if (driver->perProcessReplyQueue[pidx]) {
      // Already open!
      return ios::Error::Busy;
   }

   if (!replyQueue || numReplies != IPCKDriverReplyQueue::Size) {
      return ios::Error::Invalid;
   }

   driver->perProcessNumUserRequests[pidx] = 0u;
   driver->perProcessReplyQueue[pidx] = replyQueue;
   driver->perProcessCallbacks[pidx] = handler;
   std::memset(replyQueue.getRawPointer(), 0, sizeof(IPCKDriverReplyQueue));
   return ios::Error::OK;
}


/**
 * Close the current core's IPCKDriver for the current userland process.
 */
ios::Error
ipckDriverUserClose()
{
   auto driver = internal::ipckDriverGetInstance();
   if (!driver) {
      return ios::Error::OK;
   }

   // TODO: Cleanup any pending requests

   return ios::Error::OK;
}


/**
 * Submit a user IPC request.
 */
ios::Error
ipckDriverUserSubmitRequest(virt_ptr<IPCKDriverRequest> request)
{
   auto driver = internal::ipckDriverGetInstance();
   if (!driver) {
      return ios::Error::OK;
   }

   return internal::submitUserRequest(driver, request);
}


/**
 * Submit an IPC reply from IOS.
 */
void
ipckDriverIosSubmitReply(phys_ptr<ios::IpcRequest> reply)
{
   auto coreId = reply->cpuId - ios::CpuId::PPC0;

   sIpcMutex.lock();
   sPendingResponses[coreId].push_back(reply);
   sIpcMutex.unlock();

   cpu::interrupt(coreId, cpu::IPC_INTERRUPT);
}


namespace internal
{

virt_ptr<IPCKDriver>
ipckDriverGetInstance()
{
   return virt_addrof(sIpckDriverData->drivers[cpu::this_core::id()]);
}


ios::Error
ipckDriverAllocateRequestBlock(RamPartitionId clientProcessId,
                               RamPartitionId loaderProcessId,
                               virt_ptr<IPCKDriver> driver,
                               virt_ptr<IPCKDriverRequestBlock> *outRequestBlock,
                               ios::Handle handle,
                               ios::Command command,
                               IPCKDriverHostAsyncCallbackFn asyncCallback,
                               virt_ptr<void> asyncCallbackData)
{
   auto error = IPCKDriver_FIFOPop(virt_addrof(driver->freeFifo), outRequestBlock);
   if (error < ios::Error::OK) {
      if (error == ios::Error::QEmpty) {
         return ios::Error::QFull;
      }

      return error;
   }

   auto requestBlock = *outRequestBlock;
   requestBlock->flags = requestBlock->flags.value()
      .unk_0x0C00(0)
      .replyState(IPCKDriverRequestBlock::WaitReply)
      .requestState(IPCKDriverRequestBlock::Allocated)
      .clientProcessId(clientProcessId)
      .loaderProcessId(loaderProcessId);

   requestBlock->asyncCallback->func = asyncCallback;
   requestBlock->asyncCallbackData = asyncCallbackData;
   requestBlock->userRequest = nullptr;

   auto request = requestBlock->request;
   std::memset(virt_addrof(request->request.args).getRawPointer(), 0, sizeof(request->request.args));

   request->request.clientPid = static_cast<int32_t>(loaderProcessId);
   request->request.handle = handle;
   request->request.command = command;
   request->request.flags = 0u;
   request->request.reply = ios::Error::OK;
   request->request.cpuId = static_cast<ios::CpuId>(cpu::this_core::id() + 1);

   if (clientProcessId != RamPartitionId::Kernel) {
      request->request.titleId = getCurrentTitleId();
   }

   request->prevHandle = handle;
   request->prevCommand = command;
   return ios::Error::OK;
}


void
ipckDriverFreeRequestBlock(virt_ptr<IPCKDriver> driver,
                           virt_ptr<IPCKDriverRequestBlock> requestBlock)
{
   requestBlock->flags = requestBlock->flags.value()
      .requestState(IPCKDriverRequestBlock::Unallocated)
      .replyState(IPCKDriverRequestBlock::WaitReply)
      .unk_0x0C00(0);
   requestBlock->asyncCallback->func = nullptr;
   requestBlock->asyncCallbackData = nullptr;
   requestBlock->userRequest = nullptr;
   IPCKDriver_FIFOPush(virt_addrof(driver->freeFifo), requestBlock);
}


ios::Error
ipckDriverSubmitRequest(virt_ptr<IPCKDriver> driver,
                        virt_ptr<IPCKDriverRequestBlock> requestBlock)
{
   auto error = IPCKDriver_FIFOPush(virt_addrof(driver->outboundFIFO),
                                    requestBlock);
   if (error < ios::Error::OK) {
      return error;
   }

   if (driver->state != IPCKDriverState::Open) {
      return ios::Error::NotReady;
   }

   error = IPCKDriver_FIFOPop(virt_addrof(driver->outboundFIFO),
                              &requestBlock);
   if (error < ios::Error::OK) {
      return error;
   }

   ios::kernel::submitIpcRequest(
      effectiveToPhysical(virt_addrof(requestBlock->request->request)));
   return ios::Error::OK;
}


static ios::Error
allocateUserRequestBlock(RamPartitionId clientProcessId,
                         RamPartitionId loaderProcessId,
                         virt_ptr<IPCKDriver> driver,
                         virt_ptr<IPCKDriverRequestBlock> *outRequestBlock,
                         virt_ptr<IPCKDriverRequest> userRequest)
{
   auto error = ipckDriverAllocateRequestBlock(clientProcessId,
                                               loaderProcessId,
                                               driver,
                                               outRequestBlock,
                                               0,
                                               ios::Command::Invalid,
                                               nullptr, nullptr);

   if (error >= ios::Error::OK) {
      auto requestBlock = *outRequestBlock;
      requestBlock->userRequest = userRequest;
      requestBlock->flags = requestBlock->flags.value()
         .clientProcessId(clientProcessId);
   }

   return error;
}


static ios::Error
processLoaderOrUserRequest(virt_ptr<IPCKDriver> driver,
                           virt_ptr<IPCKDriverRequestBlock> requestBlock,
                           bool isLoader)
{
   auto request = requestBlock->request;
   request->prevHandle = request->request.handle;
   request->prevCommand = request->request.command;
   request->request.flags = 0u;
   request->request.reply = ios::Error::OK;
   request->request.cpuId = static_cast<ios::CpuId>(cpu::this_core::id() + 1);

   if (!isLoader) {
      request->request.clientPid = static_cast<int32_t>(getCurrentRampid());
      request->request.titleId = getCurrentTitleId();
   }

   switch (request->request.command) {
   case ios::Command::Open:
      request->request.args.open.name =
         phys_cast<const char *>(effectiveToPhysical(request->buffer1));
      break;
   case ios::Command::Read:
      request->request.args.read.data = effectiveToPhysical(request->buffer1);
      break;
   case ios::Command::Write:
      request->request.args.write.data = effectiveToPhysical(request->buffer1);
      break;
   case ios::Command::Ioctl:
      if (request->buffer1) {
         request->request.args.ioctl.inputBuffer =
            phys_cast<const char *>(effectiveToPhysical(request->buffer1));
      } else {
         request->request.args.ioctl.inputBuffer = nullptr;
      }

      if (request->buffer2) {
         request->request.args.ioctl.outputBuffer =
            phys_cast<const char *>(effectiveToPhysical(request->buffer2));
      } else {
         request->request.args.ioctl.outputBuffer = nullptr;
      }
      break;
   case ios::Command::Ioctlv:
   {
      auto &ioctlv = request->request.args.ioctlv;

      if (request->buffer1) {
         ioctlv.vecs =
            phys_cast<ios::IoctlVec *>(effectiveToPhysical(request->buffer1));
      } else {
         ioctlv.vecs = nullptr;
      }

      for (auto i = 0u; i < ioctlv.numVecIn + ioctlv.numVecOut; ++i) {
         if (!ioctlv.vecs[i].vaddr) {
            continue;
         }

         ioctlv.vecs[i].paddr = effectiveToPhysical(ioctlv.vecs[i].vaddr);
      }
      break;
   }
   default:
      break;
   }

   return ios::Error::OK;
}


static ios::Error
submitUserOrLoaderRequest(virt_ptr<IPCKDriver> driver,
                          virt_ptr<IPCKDriverRequest> userRequest,
                          bool isLoader)
{
   auto error = ios::Error::OK;
   auto rampid = getCurrentRampid();
   auto pidx = static_cast<uint32_t>(rampid);

   if (!driver) {
      error = ios::Error::Invalid;
   } else {
      if (driver->perProcessNumUserRequests[pidx] +
          driver->perProcessNumLoaderRequests[pidx] >= IPCKRequestsPerProcess) {
         error = ios::Error::QFull;
      } else {
         virt_ptr<IPCKDriverRequestBlock> requestBlock;
         error = allocateUserRequestBlock(isLoader ? RamPartitionId::Kernel : rampid,
                                          isLoader ? rampid : RamPartitionId::Invalid,
                                          driver,
                                          &requestBlock,
                                          userRequest);
         if (error >= ios::Error::OK) {
            std::memcpy(requestBlock->request.getRawPointer(),
                        userRequest.getRawPointer(),
                        0x48u);

            error = processLoaderOrUserRequest(driver, requestBlock, isLoader);
            if (error >= ios::Error::OK) {
               error = ipckDriverSubmitRequest(driver, requestBlock);
            }

            if (isLoader) {
               driver->perProcessNumLoaderRequests[pidx]++;
            } else {
               driver->perProcessNumUserRequests[pidx]++;
            }

            if (error < ios::Error::OK) {
               ipckDriverFreeRequestBlock(driver, requestBlock);
            }
         }
      }
   }

   driver->perProcessLastError[pidx] = error;
   return ios::Error::OK;
}


ios::Error
submitUserRequest(virt_ptr<IPCKDriver> driver,
                  virt_ptr<IPCKDriverRequest> userRequest)
{
   return submitUserOrLoaderRequest(driver, userRequest, false);
}


ios::Error
submitLoaderRequest(virt_ptr<IPCKDriver> driver,
                    virt_ptr<IPCKDriverRequest> userRequest)
{
   return submitUserOrLoaderRequest(driver, userRequest, true);
}


static ios::Error
defensiveProcessIncomingMessagePointer(virt_ptr<IPCKDriver> driver,
                                       virt_ptr<IPCKDriverRequest> request,
                                       virt_ptr<IPCKDriverRequestBlock> *outRequestBlock)
{
   auto index = request - driver->requestsBuffer;
   if (index >= IPCKRequestsPerCore || index < 0) {
      return ios::Error::Invalid;
   }

   if (driver->requestBlocks[index].request != request) {
      return ios::Error::Invalid;
   }

   auto requestBlock = virt_addrof(driver->requestBlocks[index]);
   auto flags = requestBlock->flags.value();
   if (flags.requestState() == IPCKDriverRequestBlock::Unallocated) {
      return ios::Error::Invalid;
   }

   requestBlock->flags =
      flags.replyState(IPCKDriverRequestBlock::ReceivedReply);
   *outRequestBlock = requestBlock;
   return ios::Error::OK;
}


static void
processReply(virt_ptr<IPCKDriver> driver,
             phys_ptr<ios::IpcRequest> reply)
{
   if (driver->state < IPCKDriverState::Open) {
      return;
   }

   auto requestBlock = virt_ptr<IPCKDriverRequestBlock> { nullptr };
   auto request = virt_cast<IPCKDriverRequest *>(physicalToEffectiveCached(phys_cast<phys_addr>(reply)));
   auto error = defensiveProcessIncomingMessagePointer(driver, request, &requestBlock);
   if (error < ios::Error::OK) {
      return;
   }

   if (requestBlock->asyncCallback->func) {
      requestBlock->asyncCallback->func(reply->reply, requestBlock->asyncCallbackData);
      ipckDriverFreeRequestBlock(driver, requestBlock);
   } else {
      auto isLoader = false;
      auto flags = requestBlock->flags.value();
      auto processId = flags.clientProcessId();

      if (flags.loaderProcessId() != RamPartitionId::Invalid &&
          flags.loaderProcessId() != RamPartitionId::Kernel) {
         processId = flags.loaderProcessId();
         isLoader = true;
      }

      auto pidx = static_cast<unsigned>(processId);
      if (isLoader) {
         IPCKDriver_FIFOPush(virt_addrof(driver->perProcessLoaderReply[pidx]),
                             requestBlock);
      } else {
         IPCKDriver_FIFOPush(virt_addrof(driver->perProcessUserReply[pidx]),
                             requestBlock);
      }
   }
}


static void
dispatchUserRepliesCallback(virt_ptr<IPCKDriver> driver,
                            virt_ptr<Context> interruptedContext,
                            uint32_t pidx)
{
   // Fill the process reply queue
   auto requestBlock = virt_ptr<IPCKDriverRequestBlock> { nullptr };
   auto replyQueue = driver->perProcessReplyQueue[pidx];

   while (replyQueue->numReplies < replyQueue->replies.size()) {
      auto error = IPCKDriver_FIFOPop(virt_addrof(driver->perProcessUserReply[pidx]),
                                      &requestBlock);
      if (error < ios::Error::OK) {
         break;
      }

      std::memcpy(requestBlock->userRequest.getRawPointer(),
                  requestBlock->request.getRawPointer(),
                  0x48u);

      replyQueue->replies[replyQueue->numReplies] = requestBlock->userRequest;
      replyQueue->numReplies++;

      driver->perProcessNumUserRequests[pidx]--;
      ipckDriverFreeRequestBlock(driver, requestBlock);
   }

   for (auto i = replyQueue->numReplies; i < replyQueue->replies.size(); ++i) {
      replyQueue->replies[i] = nullptr;
   }

   // Call the user callback
   if (driver->perProcessCallbacks[pidx]) {
      driver->perProcessCallbacks[pidx](driver->interruptType, interruptedContext);
   }
}


static void
ipckDriverHandleInterrupt(InterruptType type,
                          virt_ptr<Context> interruptedContext)
{
   auto driver = ipckDriverGetInstance();
   auto responses = std::vector<phys_ptr<ios::IpcRequest>> { };

   // Get the pending replies
   sIpcMutex.lock();
   sPendingResponses[driver->coreId].swap(responses);
   sIpcMutex.unlock();

   // Process replies into process queue
   for (auto response : responses) {
      processReply(driver, response);
   }

   // Dispatch any pending user replies for the current process
   auto pidx = static_cast<uint32_t>(getCurrentRampid());
   if (driver->perProcessUserReply[pidx].count) {
      dispatchUserRepliesCallback(driver, interruptedContext, pidx);
   }
}


static ios::Error
initialiseRegisters(virt_ptr<IPCKDriver> driver)
{
   switch (driver->coreId) {
   case 0:
      driver->registers.ppcMsg = virt_cast<void *>(virt_addr { 0x0D800400 });
      driver->registers.ppcCtrl = virt_cast<void *>(virt_addr { 0x0D800404 });
      driver->registers.armMsg = virt_cast<void *>(virt_addr { 0x0D800408 });
      driver->registers.ahbLt = virt_cast<void *>(virt_addr { 0x0D800444 });
      driver->registers.unkMaybeFlags = 0x40000000u;
      break;
   case 1:
      driver->registers.ppcMsg = virt_cast<void *>(virt_addr { 0x0D800410 });
      driver->registers.ppcCtrl = virt_cast<void *>(virt_addr { 0x0D800414 });
      driver->registers.armMsg = virt_cast<void *>(virt_addr { 0x0D800418 });
      driver->registers.ahbLt = virt_cast<void *>(virt_addr { 0x0D800454 });
      driver->registers.unkMaybeFlags = 0x10000000u;
      break;
   case 2:
      driver->registers.ppcMsg = virt_cast<void *>(virt_addr { 0x0D800420 });
      driver->registers.ppcCtrl = virt_cast<void *>(virt_addr { 0x0D800424 });
      driver->registers.armMsg = virt_cast<void *>(virt_addr { 0x0D800428 });
      driver->registers.ahbLt = virt_cast<void *>(virt_addr { 0x0D800464 });
      driver->registers.unkMaybeFlags = 0x04000000u;
      break;
   }

   return ios::Error::OK;
}


ios::Error
ipckDriverInit()
{
   auto driver = ipckDriverGetInstance();
   std::memset(driver.getRawPointer(), 0, sizeof(IPCKDriver));

   driver->coreId = cpu::this_core::id();

   switch (driver->coreId) {
   case 0:
      driver->interruptType = InterruptType::IpcPpc0;
      driver->requestsBuffer = virt_addrof(sIpckDriverData->requestBuffer);
      break;
   case 1:
      driver->interruptType = InterruptType::IpcPpc1;
      driver->requestsBuffer =
         virt_addrof(sIpckDriverData->requestBuffer) + IPCKRequestsPerCore;
      break;
   case 2:
      driver->interruptType = InterruptType::IpcPpc2;
      driver->requestsBuffer =
         virt_addrof(sIpckDriverData->requestBuffer) + IPCKRequestsPerCore * 2;
      break;
   }

   auto error = initialiseRegisters(driver);
   if (error < ios::Error::OK) {
      return error;
   }

   std::memset(driver->requestsBuffer.getRawPointer(), 0,
               sizeof(IPCKDriverRequest) * IPCKRequestsPerCore);
   driver->state = IPCKDriverState::Initialised;

   // TODO: Register proc action callback to cleanup IPCKDriver for process
   return ios::Error::OK;
}


static ios::Error
initialiseResourceBuffers(virt_ptr<IPCKDriver> driver)
{
   for (auto i = 0u; i < IPCKRequestsPerCore; ++i) {
      // Allocate memory to hold our host function pointer, this is because the
      // host function pointer will be 8 bytes on 64bit systems but we only
      // have space for 4 bytes in the structure.
      auto hostCallbackPtr = allocStaticData<IPCKDriverHostAsyncCallback>();
      hostCallbackPtr->func = nullptr;

      driver->requestBlocks[i].asyncCallback = hostCallbackPtr;
      driver->requestBlocks[i].asyncCallbackData = nullptr;
      driver->requestBlocks[i].userRequest = nullptr;
      driver->requestBlocks[i].request = virt_addrof(driver->requestsBuffer[i]);
   }

   return ios::Error::OK;
}


ios::Error
ipckDriverOpen()
{
   auto driver = ipckDriverGetInstance();
   if (driver->state != IPCKDriverState::Initialised &&
       driver->state != IPCKDriverState::Unknown1) {
      return ios::Error::NotReady;
   }

   auto error = initialiseResourceBuffers(driver);
   if (error < ios::Error::OK) {
      return error;
   }

   IPCKDriver_FIFOInit(virt_addrof(driver->freeFifo));
   IPCKDriver_FIFOInit(virt_addrof(driver->outboundFIFO));

   for (auto i = 0u; i < NumRamPartitions; ++i) {
      IPCKDriver_FIFOInit(virt_addrof(driver->perProcessUserReply[i]));
      IPCKDriver_FIFOInit(virt_addrof(driver->perProcessLoaderReply[i]));
   }

   for (auto i = 0u; i < IPCKRequestsPerCore; ++i) {
      IPCKDriver_FIFOPush(virt_addrof(driver->freeFifo),
                          virt_addrof(driver->requestBlocks[i]));
   }

   driver->unk0x04++;
   setUserModeInterruptHandler(driver->interruptType,
                               ipckDriverHandleInterrupt);

   driver->state = IPCKDriverState::Open;
   return ios::Error::OK;
}


void
initialiseStaticIpckDriverData()
{
   sIpckDriverData = allocStaticData<StaticIpckDriverData>();
}

} // namespace internal

} // namespace cafe::kernel

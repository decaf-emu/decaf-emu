#include "coreinit.h"
#include "coreinit_core.h"
#include "coreinit_ipcdriver.h"
#include "coreinit_messagequeue.h"
#include "coreinit_scheduler.h"
#include "coreinit_thread.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/kernel/cafe_kernel_ipckdriver.h"

namespace cafe::coreinit
{

struct StaticIpcDriverData
{
   struct PerCoreData
   {
      be2_struct<IPCDriver> driver;
      be2_struct<OSMessageQueue> queue;
      be2_array<OSMessage, 0x30> messages;
      be2_array<IPCKDriverRequest, IPCBufferCount> ipcBuffers;
      be2_struct<OSThread> thread;
      be2_array<char, 16> threadName;
      be2_array<uint8_t, 0x4000> threadStack;
   };

   be2_array<PerCoreData, CoreCount> perCoreData;
   be2_array<char, 32> submitEventName;
};

static virt_ptr<StaticIpcDriverData> sIpcDriverData = nullptr;
static OSThreadEntryPointFn sIpcDriverThreadEntry = nullptr;

namespace internal
{

void
ipcDriverProcessReplies(kernel::InterruptType type,
                        virt_ptr<kernel::Context> interruptedContext);

} // namespace internal


/**
 * Initialise the IPC driver.
 */
void
IPCDriverInit()
{
   auto coreId = OSGetCoreId();
   auto &perCoreData = sIpcDriverData->perCoreData[coreId];

   auto driver = virt_addrof(perCoreData.driver);
   OSInitEvent(virt_addrof(driver->waitFreeFifoEvent), FALSE, OSEventMode::AutoReset);
   driver->status = IPCDriverStatus::Initialised;
   driver->coreId = coreId;
   driver->ipcBuffers = virt_addrof(perCoreData.ipcBuffers);

   OSInitMessageQueue(virt_addrof(perCoreData.queue),
                      virt_addrof(perCoreData.messages),
                      static_cast<int32_t>(perCoreData.messages.size()));

   auto thread = virt_addrof(perCoreData.thread);
   auto stack = virt_addrof(perCoreData.threadStack);
   auto stackSize = perCoreData.threadStack.size();
   coreinit__OSCreateThreadType(thread,
                                sIpcDriverThreadEntry,
                                driver->coreId,
                                nullptr,
                                virt_cast<uint32_t *>(stack + stackSize),
                                static_cast<uint32_t>(stackSize),
                                15,
                                static_cast<OSThreadAttributes>(1 << driver->coreId),
                                OSThreadType::Driver);

   perCoreData.threadName = fmt::format("IPC Core {}", coreId);
   OSSetThreadName(thread, virt_addrof(perCoreData.threadName));
   OSResumeThread(thread);
}


/**
 * Open the IPC driver.
 *
 * \retval IOSError::OK
 * Success.
 *
 * \retval IOSError::NotReady
 * The IPC driver status must be Closed or Initialised.
 */
IOSError
IPCDriverOpen()
{
   auto driver = internal::getIPCDriver();

   // Verify driver state
   if (driver->status != IPCDriverStatus::Closed &&
       driver->status != IPCDriverStatus::Initialised) {
      return IOSError::NotReady;
   }

   // Initialise requests
   for (auto i = 0u; i < IPCBufferCount; ++i) {
      auto &request = driver->requests[i];
      request.ipcBuffer = virt_addrof(driver->ipcBuffers[i]);
      request.asyncCallback = nullptr;
      request.asyncContext = nullptr;
   }

   driver->initialisedRequests = TRUE;

   // Initialise FIFO
   internal::ipcDriverFifoInit(virt_addrof(driver->freeFifo));
   internal::ipcDriverFifoInit(virt_addrof(driver->outboundFifo));

   // Push all items into free queue
   for (auto i = 0u; i < IPCBufferCount; ++i) {
      internal::ipcDriverFifoPush(virt_addrof(driver->freeFifo),
                                  virt_addrof(driver->requests[i]));
   }

   // Open the ipck driver
   auto error =
      kernel::ipckDriverUserOpen(driver->replyQueue.replies.size(),
                                 virt_addrof(driver->replyQueue),
                                 internal::ipcDriverProcessReplies);
   if (error == ios::Error::OK) {
      driver->status = IPCDriverStatus::Open;
   }

   return error;
}


/**
 * Close the IPC driver.
 *
 * \retval IOSError::OK
 * Success.
 */
IOSError
IPCDriverClose()
{
   auto &driver = sIpcDriverData->perCoreData[OSGetCoreId()].driver;
   driver.status = IPCDriverStatus::Closed;
   return IOSError::OK;
}


namespace internal
{


/**
 * Get the IPC driver for the current core
 */
virt_ptr<IPCDriver>
getIPCDriver()
{

   auto coreId = OSGetCoreId();
   return virt_addrof(sIpcDriverData->perCoreData[coreId].driver);
}


/**
 * Initialise IPCDriverFIFO
 */
void
ipcDriverFifoInit(virt_ptr<IPCDriverFIFO> fifo)
{
   fifo->pushIndex = 0;
   fifo->popIndex = -1;
   fifo->count = 0;
   fifo->maxCount = 0;

   for (auto i = 0; i < IPCBufferCount; ++i) {
      fifo->requests[i] = nullptr;
   }
}


/**
 * Push a request into an IPCDriverFIFO structure
 *
 * \retval IOSError::OK Success
 * \retval IOSError::QFull There was no free space in the queue to push the request.
 */
IOSError
ipcDriverFifoPush(virt_ptr<IPCDriverFIFO> fifo,
                  virt_ptr<IPCDriverRequest> request)
{
   if (fifo->pushIndex == fifo->popIndex) {
      return IOSError::QFull;
   }

   fifo->requests[fifo->pushIndex] = request;

   if (fifo->popIndex == -1) {
      fifo->popIndex = fifo->pushIndex;
   }

   fifo->count += 1;
   fifo->pushIndex = (fifo->pushIndex + 1) % IPCBufferCount;

   if (fifo->count > fifo->maxCount) {
      fifo->maxCount = fifo->count;
   }

   return IOSError::OK;
}


/**
 * Pop a request into an IPCDriverFIFO structure.
 *
 * \retval IOSError::OK
 * Success
 *
 * \retval IOSError::QEmpty
 * There was no requests to pop from the queue.
 */
IOSError
ipcDriverFifoPop(virt_ptr<IPCDriverFIFO> fifo,
                 virt_ptr<IPCDriverRequest> *requestOut)
{
   if (fifo->popIndex == -1) {
      return IOSError::QEmpty;
   }

   auto request = fifo->requests[fifo->popIndex];
   fifo->count -= 1;

   if (fifo->count == 0) {
      fifo->popIndex = -1;
   } else {
      fifo->popIndex = (fifo->popIndex + 1) % IPCBufferCount;
   }

   *requestOut = request;
   return IOSError::OK;
}


/**
 * Allocates and initialises a IPCDriverRequest.
 *
 * This function can block with OSWaitEvent until there is a free request to
 * pop from the freeFifo queue.
 *
 * \return
 * Returns IOSError::OK on success, an IOSError code otherwise.
 */
IOSError
ipcDriverAllocateRequest(virt_ptr<IPCDriver> driver,
                         virt_ptr<IPCDriverRequest> *requestOut,
                         IOSHandle handle,
                         IOSCommand command,
                         uint32_t requestUnk0x04,
                         IOSAsyncCallbackFn asyncCallback,
                         virt_ptr<void> asyncContext)
{
   virt_ptr<IPCDriverRequest> request = nullptr;
   auto error = IOSError::OK;

   do {
      error = ipcDriverFifoPop(virt_addrof(driver->freeFifo), &request);

      if (error) {
         driver->failedAllocateRequestBlock += 1;

         if (error == IOSError::QEmpty) {
            driver->waitingFreeFifo = TRUE;
            OSWaitEvent(virt_addrof(driver->waitFreeFifoEvent));
         }
      }
   } while (error == IOSError::QEmpty);

   if (error != IOSError::OK) {
      return error;
   }

   request->allocated = TRUE;
   request->unk0x04 = requestUnk0x04;
   request->asyncCallback = asyncCallback;
   request->asyncContext = asyncContext;

   auto ipcBuffer = request->ipcBuffer;
   std::memset(virt_addrof(ipcBuffer->request).getRawPointer(),
               0,
               sizeof(kernel::IpcRequest));
   ipcBuffer->request.command = command;
   ipcBuffer->request.handle = handle;
   ipcBuffer->request.flags = 0u;
   ipcBuffer->request.clientPid = 0;
   ipcBuffer->request.reply = IOSError::OK;

   *requestOut = request;
   return IOSError::OK;
}


/**
 * Free a IPCDriverRequest.
 *
 * \retval IOSError::OK
 * Success.
 *
 * \retval IOSError::QFull
 * The driver's freeFifo queue was full thus we were unable to free the request.
 */
IOSError
ipcDriverFreeRequest(virt_ptr<IPCDriver> driver,
                     virt_ptr<IPCDriverRequest> request)
{
   auto error = ipcDriverFifoPush(virt_addrof(driver->freeFifo), request);
   request->allocated = FALSE;

   if (error != IOSError::OK) {
      driver->failedFreeRequestBlock += 1;
   }

   return error;
}


/**
 * Submits an IPCDriverRequest to the kernel IPC driver.
 *
 * \retval IOSError::OK
 * Success.
 */
IOSError
ipcDriverSubmitRequest(virt_ptr<IPCDriver> driver,
                       virt_ptr<IPCDriverRequest> request)
{
   // TODO: sIpcDriverData->submitEventName = "{ IPC Synchronous }";
   OSInitEventEx(virt_addrof(request->finishEvent),
                 FALSE,
                 OSEventMode::AutoReset,
                 virt_addrof(sIpcDriverData->submitEventName));
   driver->requestsSubmitted++;

   kernel::ipckDriverUserSubmitRequest(request->ipcBuffer);
   return IOSError::OK;
}


/**
 * Blocks and waits for a response to an IPCDriverRequest.
 *
 * \return
 * Returns IOSError::OK or an IOSHandle on success, or an IOSError code otherwise.
 */
IOSError
ipcDriverWaitResponse(virt_ptr<IPCDriver> driver,
                      virt_ptr<IPCDriverRequest> request)
{
   OSWaitEvent(virt_addrof(request->finishEvent));
   auto response = request->ipcBuffer->request.reply;
   ipcDriverFreeRequest(driver, request);
   OSSignalEventAll(virt_addrof(driver->waitFreeFifoEvent));
   return response;
}


/**
 * Callback by kernel IPC driver to indicate there are pending replies to process.
 */
void
ipcDriverProcessReplies(kernel::InterruptType type,
                        virt_ptr<kernel::Context> interruptedContext)
{
   disableScheduler();
   auto driver = getIPCDriver();
   auto &coreData = sIpcDriverData->perCoreData[driver->coreId];

   for (auto i = 0u; i < driver->replyQueue.numReplies; ++i) {
      auto buffer = driver->replyQueue.replies[i];
      auto index = static_cast<uint32_t>(buffer - driver->ipcBuffers);
      decaf_check(index >= 0);
      decaf_check(index <= IPCBufferCount);

      auto &request = driver->requests[index];
      decaf_check(request.ipcBuffer == buffer);

      if (!request.asyncCallback) {
         OSSignalEvent(virt_addrof(request.finishEvent));
      } else {
         StackObject<OSMessage> message;
         message->message = virt_cast<void *>(virt_func_cast<virt_addr>(request.asyncCallback));
         message->args[0] = static_cast<uint32_t>(request.ipcBuffer->request.reply.value());
         message->args[1] = static_cast<uint32_t>(virt_cast<virt_addr>(request.asyncContext));
         message->args[2] = 0u;
         OSSendMessage(virt_addrof(coreData.queue), message, OSMessageFlags::None);
         ipcDriverFreeRequest(driver, virt_addrof(request));
      }

      driver->requestsProcessed++;
      driver->replyQueue.replies[i] = nullptr;
   }

   driver->replyQueue.numReplies = 0u;
   enableScheduler();
}


static uint32_t
ipcDriverThreadEntry(uint32_t coreId,
                     virt_ptr<void>)
{
   StackObject<OSMessage> msg;
   auto &coreData = sIpcDriverData->perCoreData[coreId];

   while (true) {
      OSReceiveMessage(virt_addrof(coreData.queue), msg, OSMessageFlags::Blocking);

      if (msg->args[2]) {
         // Received shutdown message
         break;
      }

      // Received callback message
      auto callback = virt_func_cast<IOSAsyncCallbackFn>(virt_cast<virt_addr>(msg->message));
      auto error = static_cast<IOSError>(msg->args[0]);
      auto context = virt_cast<void *>(virt_addr { msg->args[1].value() });
      cafe::invoke(cpu::this_core::state(),
                   callback,
                   error,
                   context);
   }

   IPCDriverClose();
   return 0;
}

} // namespace internal

void
Library::registerIpcDriverSymbols()
{
   RegisterFunctionExport(IPCDriverInit);
   RegisterFunctionExport(IPCDriverOpen);
   RegisterFunctionExport(IPCDriverClose);

   RegisterDataInternal(sIpcDriverData);
   RegisterFunctionInternal(internal::ipcDriverThreadEntry, sIpcDriverThreadEntry);
}

} // namespace cafe::coreinit

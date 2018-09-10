#include "cafe_loader_heap.h"
#include "cafe_loader_ipcldriver.h"

#include "cafe/kernel/cafe_kernel_ipckdriver.h"
#include "cafe/cafe_stackobject.h"

#include <libcpu/cpu.h>

using namespace cafe::kernel;

namespace cafe::loader::internal
{

struct StaticIpclData
{
   be2_array<IPCLDriver, 3> drivers;
   be2_array<IPCKDriverRequest, IPCLBufferCount * 3> ipclResourceRequestBuffer;
};

static virt_ptr<StaticIpclData>
sIpclData;

ios::Error
IPCLDriver_Init()
{
   virt_ptr<IPCLDriver> driver;
   IPCLDriver_GetInstance(&driver);

   std::memset(driver.getRawPointer(), 0, sizeof(IPCLDriver));
   driver->coreId = cpu::this_core::id();
   driver->ipckRequestBuffer = virt_addrof(sIpclData->ipclResourceRequestBuffer[driver->coreId]);
   driver->status = IPCLDriverStatus::Initialised;

   std::memset(driver->ipckRequestBuffer.getRawPointer(), 0, sizeof(IPCKDriverRequest) * IPCLBufferCount);

   return ios::Error::OK;
}

ios::Error
IPCLDriver_InitRequestParameterBlocks(virt_ptr<IPCLDriver> driver)
{
   for (auto i = 0u; i < IPCLBufferCount; ++i) {
      auto ipckRequestBuffer = virt_addrof(driver->ipckRequestBuffer[i]);
      auto &request = driver->requests[i];
      request.ipckRequestBuffer = ipckRequestBuffer;
      request.asyncCallback = nullptr;
      request.asyncCallbackData = nullptr;
   }

   return ios::Error::OK;
}

ios::Error
IPCLDriver_Open()
{
   virt_ptr<IPCLDriver> driver;
   IPCLDriver_GetInstance(&driver);

   if (driver->status != IPCLDriverStatus::Closed && driver->status != IPCLDriverStatus::Initialised) {
      return ios::Error::NotReady;
   }

   IPCLDriver_InitRequestParameterBlocks(driver);

   IPCLDriver_FIFOInit(virt_addrof(driver->freeFifo));
   IPCLDriver_FIFOInit(virt_addrof(driver->outboundFifo));

   for (auto i = 0u; i < IPCLBufferCount; ++i) {
      IPCLDriver_FIFOPush(virt_addrof(driver->freeFifo),
                          virt_addrof(driver->requests[i]));
   }

   auto error = kernel::ipckDriverLoaderOpen();
   if (error < ios::Error::OK) {
      return error;
   }

   driver->status = IPCLDriverStatus::Open;
   return ios::Error::OK;
}

ios::Error
IPCLDriver_GetInstance(virt_ptr<IPCLDriver> *outDriver)
{
   auto driver = virt_addrof(sIpclData->drivers[cpu::this_core::id()]);
   *outDriver = driver;

   if (driver->status < IPCLDriverStatus::Open) {
      return ios::Error::NotReady;
   } else {
      return ios::Error::OK;
   }
}

ios::Error
IPCLDriver_AllocateRequestBlock(virt_ptr<IPCLDriver> driver,
                                virt_ptr<IPCLDriverRequest> *outRequest,
                                ios::Handle handle,
                                ios::Command command,
                                IPCLAsyncCallbackFn callback,
                                virt_ptr<void> callbackContext)
{
   virt_ptr<IPCLDriverRequest> request;
   auto error = IPCLDriver_FIFOPop(virt_addrof(driver->freeFifo), &request);
   if (error < ios::Error::OK) {
      driver->failedAllocateRequestBlock++;
      return error;
   }

   // Initialise IPCLDriverRequest
   request->allocated = TRUE;
   request->asyncCallback = callback;
   request->asyncCallbackData = callbackContext;

   // Initialise IPCKDriverRequest
   auto ipckRequest = request->ipckRequestBuffer;
   std::memset(virt_addrof(ipckRequest->request.args).getRawPointer(), 0, sizeof(ipckRequest->request.args));
   ipckRequest->request.command = command;
   ipckRequest->request.handle = handle;
   ipckRequest->request.flags = 0u;
   ipckRequest->request.clientPid = 0;
   ipckRequest->request.reply = ios::Error::OK;

   *outRequest = request;
   return ios::Error::OK;
}

ios::Error
IPCLDriver_FreeRequestBlock(virt_ptr<IPCLDriver> driver,
                            virt_ptr<IPCLDriverRequest> request)
{
   auto error = IPCLDriver_FIFOPush(virt_addrof(driver->freeFifo), request);
   request->allocated = FALSE;

   if (error < ios::Error::OK) {
      driver->failedFreeRequestBlock++;
   }

   return error;
}

static ios::Error
defensiveProcessIncomingMessagePointer(virt_ptr<IPCLDriver> driver,
                                       virt_ptr<cafe::kernel::IPCKDriverRequest> ipckRequest,
                                       virt_ptr<IPCLDriverRequest> *outIpclRequest)
{
   if (ipckRequest < driver->ipckRequestBuffer) {
      return ios::Error::Invalid;
   }

   auto index = ipckRequest - driver->ipckRequestBuffer;
   if (index >= IPCLBufferCount) {
      return ios::Error::Invalid;
   }

   if (!driver->requests[index].allocated) {
      driver->invalidReplyMessagePointerNotAlloc++;
      return ios::Error::Invalid;
   }

   *outIpclRequest = virt_addrof(driver->requests[index]);
   return ios::Error::OK;
}

static void
ipclProcessReply(virt_ptr<IPCLDriver> driver,
                 virt_ptr<IPCLDriverRequest> request)
{
   driver->repliesReceived++;

   auto ipckRequest = request->ipckRequestBuffer;
   switch (ipckRequest->request.command) {
   case ios::Command::Open:
      if (request->asyncCallback) {
         if (ipckRequest->request.reply < ios::Error::OK) {
            driver->iosOpenAsyncRequestFail++;
         } else {
            driver->iosOpenAsyncRequestSuccess++;
         }
      }
      break;
   case ios::Command::Close:
      if (request->asyncCallback) {
         if (ipckRequest->request.reply < ios::Error::OK) {
            driver->iosCloseAsyncRequestFail++;
         } else {
            driver->iosCloseAsyncRequestSuccess++;
         }
      }
      break;
   case ios::Command::Read:
      decaf_check(ipckRequest->buffer1 || !ipckRequest->request.args.read.length);

      if (request->asyncCallback) {
         if (ipckRequest->request.reply < ios::Error::OK) {
            driver->iosReadAsyncRequestFail++;
         } else {
            driver->iosReadAsyncRequestSuccess++;
         }
      }
      break;
   case ios::Command::Write:
      decaf_check(ipckRequest->buffer1 || !ipckRequest->request.args.write.length);

      if (request->asyncCallback) {
         if (ipckRequest->request.reply < ios::Error::OK) {
            driver->iosWriteAsyncRequestFail++;
         } else {
            driver->iosWriteAsyncRequestSuccess++;
         }
      }
      break;
   case ios::Command::Seek:
      if (request->asyncCallback) {
         if (ipckRequest->request.reply < ios::Error::OK) {
            driver->iosSeekAsyncRequestFail++;
         } else {
            driver->iosSeekAsyncRequestSuccess++;
         }
      }
      break;
   case ios::Command::Ioctl:
      decaf_check(ipckRequest->buffer1 || !ipckRequest->request.args.ioctl.inputLength);
      decaf_check(ipckRequest->buffer2 || !ipckRequest->request.args.ioctl.outputLength);

      if (request->asyncCallback) {
         if (ipckRequest->request.reply < ios::Error::OK) {
            driver->iosIoctlAsyncRequestFail++;
         } else {
            driver->iosIoctlAsyncRequestSuccess++;
         }
      }
      break;
   case ios::Command::Ioctlv:
      decaf_check(ipckRequest->buffer1 ||
                  (ipckRequest->request.args.ioctlv.numVecIn + ipckRequest->request.args.ioctlv.numVecOut) == 0);

      if (request->asyncCallback) {
         if (ipckRequest->request.reply < ios::Error::OK) {
            driver->iosIoctlvAsyncRequestFail++;
         } else {
            driver->iosIoctlvAsyncRequestSuccess++;
         }
      }
      break;
   default:
      driver->invalidReplyCommand++;
   }
}

ios::Error
IPCLDriver_ProcessReply(virt_ptr<IPCLDriver> driver,
                        virt_ptr<cafe::kernel::IPCKDriverRequest> ipckRequest)
{
   auto request = virt_ptr<IPCLDriverRequest> { nullptr };
   if (driver->status < IPCLDriverStatus::Open) {
      driver->unexpectedReplyInterrupt++;
      return ios::Error::Invalid;
   }

   auto error = defensiveProcessIncomingMessagePointer(driver, ipckRequest, &request);
   if (error < ios::Error::OK) {
      driver->invalidReplyMessagePointer++;
      return error;
   }

   ipclProcessReply(driver, request);

   if (request->asyncCallback) {
      request->asyncCallback(ipckRequest->request.reply, request->asyncCallbackData);
      IPCLDriver_FreeRequestBlock(driver, request);
      driver->asyncTransactionsCompleted++;
   }

   return error;
}

ios::Error
IPCLDriver_ProcessIOSIoctlRequest(virt_ptr<IPCLDriver> driver,
                                  virt_ptr<IPCLDriverRequest> request,
                                  uint32_t command,
                                  virt_ptr<const void> inputBuffer,
                                  uint32_t inputLength,
                                  virt_ptr<void> outputBuffer,
                                  uint32_t outputLength)
{
   auto ipckRequest = request->ipckRequestBuffer;
   ipckRequest->request.args.ioctl.request = command;
   ipckRequest->request.args.ioctl.inputBuffer = nullptr;
   ipckRequest->request.args.ioctl.inputLength = inputLength;
   ipckRequest->request.args.ioctl.outputBuffer = nullptr;
   ipckRequest->request.args.ioctl.outputLength = outputLength;

   ipckRequest->buffer1 = inputBuffer;
   ipckRequest->buffer2 = outputBuffer;
   return ios::Error::OK;
}

static ios::Error
sendFIFOToKernel(virt_ptr<IPCLDriver> driver)
{
   auto error = ios::Error::OK;
   auto poppedRequest = virt_ptr<IPCLDriverRequest> { nullptr };

   if (driver->status != IPCLDriverStatus::Open) {
      return error;
   }

   while (error == ios::Error::OK) {
      error = IPCLDriver_PeekFIFO(virt_addrof(driver->outboundFifo),
                                    virt_addrof(driver->currentSendTransaction));
      if (error < ios::Error::OK) {
         break;
      }

      driver->status = IPCLDriverStatus::Submitting;
      error = ipckDriverLoaderSubmitRequest(driver->currentSendTransaction->ipckRequestBuffer);
      if (error == ios::Error::OK) {
         IPCLDriver_FIFOPop(virt_addrof(driver->outboundFifo), &poppedRequest);
         decaf_check(poppedRequest == driver->currentSendTransaction);
      }
      driver->status = IPCLDriverStatus::Open;
   }

   return error;
}

ios::Error
IPCLDriver_SubmitRequestBlock(virt_ptr<IPCLDriver> driver,
                              virt_ptr<IPCLDriverRequest> request)
{
   // Flush out any pending requests
   sendFIFOToKernel(driver);

   auto error = IPCLDriver_FIFOPush(virt_addrof(driver->outboundFifo), request);
   if (error < ios::Error::OK) {
      driver->failedRequestSubmitOutboundFIFOFull++;

      // Try flush again
      sendFIFOToKernel(driver);
   } else {
      driver->requestsSubmitted++;
      sendFIFOToKernel(driver);
      error = ios::Error::OK;
   }

   return error;
}

ios::Error
IPCLDriver_IoctlAsync(ios::Handle handle,
                      uint32_t command,
                      virt_ptr<const void> inputBuffer,
                      uint32_t inputLength,
                      virt_ptr<void> outputBuffer,
                      uint32_t outputLength,
                      IPCLAsyncCallbackFn callback,
                      virt_ptr<void> callbackContext)
{
   if (!inputBuffer && inputLength > 0) {
      return ios::Error::InvalidArg;
   }

   if (!outputBuffer && outputLength > 0) {
      return ios::Error::InvalidArg;
   }

   auto driver = virt_ptr<IPCLDriver> { nullptr };
   auto error = IPCLDriver_GetInstance(&driver);
   if (error < ios::Error::OK) {
      return error;
   }

   auto request = virt_ptr<IPCLDriverRequest> { nullptr };
   error = IPCLDriver_AllocateRequestBlock(driver,
                                           &request,
                                           handle,
                                           ios::Command::Ioctl,
                                           callback,
                                           callbackContext);
   if (error < ios::Error::OK) {
      return error;
   }

   error = IPCLDriver_ProcessIOSIoctlRequest(driver,
                                             request,
                                             command,
                                             inputBuffer,
                                             inputLength,
                                             outputBuffer,
                                             outputLength);

   if (error >= ios::Error::OK) {
      error = IPCLDriver_SubmitRequestBlock(driver, request);
   }

   if (error < ios::Error::OK) {
      IPCLDriver_FreeRequestBlock(driver, request);
      driver->iosIoctlAsyncRequestSubmitFail++;
   } else {
      driver->iosIoctlAsyncRequestSubmitSuccess++;
   }

   return error;
}

void
initialiseIpclDriverStaticData()
{
   sIpclData = allocStaticData<StaticIpclData>();
}

} // namespace cafe::loader::internal

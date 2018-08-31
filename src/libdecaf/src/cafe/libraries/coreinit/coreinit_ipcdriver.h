#pragma once
#include "coreinit_enum.h"
#include "coreinit_event.h"
#include "coreinit_ios.h"
#include "cafe/kernel/cafe_kernel_ipckdriver.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_ipcdriver IPC Driver
 * \ingroup coreinit
 * @{
 */

constexpr auto IPCBufferCount = 0x30;
using IPCKDriverRequest = cafe::kernel::IPCKDriverRequest;

/**
 * Contains all data required for IPCDriver about an IPC request.
 *
 * ipcBuffer contains the actual data sent over IPC.
 */
struct IPCDriverRequest
{
   //! Whether the current request has been allocated.
   be2_val<BOOL> allocated;

   //! Unknown.
   be2_val<uint32_t> unk0x04;

   //! Callback to be called when reply is received.
   be2_val<IOSAsyncCallbackFn> asyncCallback;

   //! Context for asyncCallback.
   be2_virt_ptr<void> asyncContext;

   UNKNOWN(0x4);

   //! Pointer to the IPCK request.
   be2_virt_ptr<IPCKDriverRequest> ipcBuffer;

   //! OSEvent called when request is finished.
   be2_struct<OSEvent> finishEvent;
};
CHECK_OFFSET(IPCDriverRequest, 0x00, allocated);
CHECK_OFFSET(IPCDriverRequest, 0x04, unk0x04);
CHECK_OFFSET(IPCDriverRequest, 0x08, asyncCallback);
CHECK_OFFSET(IPCDriverRequest, 0x0C, asyncContext);
CHECK_OFFSET(IPCDriverRequest, 0x14, ipcBuffer);
CHECK_OFFSET(IPCDriverRequest, 0x18, finishEvent);
CHECK_SIZE(IPCDriverRequest, 0x3C);


/**
 * FIFO queue for IPCDriverRequests.
 *
 * Functions similar to a ring buffer.
 */
struct IPCDriverFIFO
{
   //! The current item index to push to
   be2_val<int32_t> pushIndex;

   //! The current item index to pop from
   be2_val<int32_t> popIndex;

   //! The number of items in the queue
   be2_val<int32_t> count;

   //! Tracks the highest amount of items there has been in the queue
   be2_val<int32_t> maxCount;

   //! Items in the queue
   be2_array<virt_ptr<IPCDriverRequest>, IPCBufferCount> requests;
};
CHECK_OFFSET(IPCDriverFIFO, 0x00, pushIndex);
CHECK_OFFSET(IPCDriverFIFO, 0x04, popIndex);
CHECK_OFFSET(IPCDriverFIFO, 0x08, count);
CHECK_OFFSET(IPCDriverFIFO, 0x0C, maxCount);
CHECK_OFFSET(IPCDriverFIFO, 0x10, requests);
CHECK_SIZE(IPCDriverFIFO, 0xD0);


/**
 * IPC driver.
 */
struct IPCDriver
{
   //! The current state of the IPCDriver
   be2_val<IPCDriverStatus> status;

   UNKNOWN(0x4);

   //! The core this driver was opened on.
   be2_val<uint32_t> coreId;

   UNKNOWN(0x4);

   //! A pointer to the memory used for IPCBuffers.
   be2_virt_ptr<IPCKDriverRequest> ipcBuffers;

   //! The current outgoing IPCDriverRequest.
   be2_virt_ptr<IPCDriverRequest> currentSendTransaction;

   be2_val<uint32_t> iosOpenRequestFail;
   be2_val<uint32_t> iosOpenRequestSuccess;
   be2_val<uint32_t> iosOpenAsyncRequestSubmitFail;
   be2_val<uint32_t> iosOpenAsyncRequestSubmitSuccess;
   be2_val<uint32_t> iosOpenAsyncRequestFail;
   be2_val<uint32_t> iosOpenAsyncRequestSuccess;
   be2_val<uint32_t> iosOpenAsyncExRequestSubmitFail;
   be2_val<uint32_t> iosOpenAsyncExRequestSubmitSuccess;
   be2_val<uint32_t> iosOpenAsyncExRequestFail;
   be2_val<uint32_t> iosOpenAsyncExRequestSuccess;
   be2_val<uint32_t> iosCloseRequestFail;
   be2_val<uint32_t> iosCloseRequestSuccess;
   be2_val<uint32_t> iosCloseAsyncRequestSubmitFail;
   be2_val<uint32_t> iosCloseAsyncRequestSubmitSuccess;
   be2_val<uint32_t> iosCloseAsyncRequestFail;
   be2_val<uint32_t> iosCloseAsyncRequestSuccess;
   be2_val<uint32_t> iosCloseAsyncExRequestSubmitFail;
   be2_val<uint32_t> iosCloseAsyncExRequestSubmitSuccess;
   be2_val<uint32_t> iosCloseAsyncExRequestFail;
   be2_val<uint32_t> iosCloseAsyncExRequestSuccess;
   be2_val<uint32_t> iosReadRequestFail;
   be2_val<uint32_t> iosReadRequestSuccess;
   be2_val<uint32_t> iosReadAsyncRequestSubmitFail;
   be2_val<uint32_t> iosReadAsyncRequestSubmitSuccess;
   be2_val<uint32_t> iosReadAsyncRequestFail;
   be2_val<uint32_t> iosReadAsyncRequestSuccess;
   be2_val<uint32_t> iosReadAsyncExRequestSubmitFail;
   be2_val<uint32_t> iosReadAsyncExRequestSubmitSuccess;
   be2_val<uint32_t> iosReadAsyncExRequestFail;
   be2_val<uint32_t> iosReadAsyncExRequestSuccess;
   be2_val<uint32_t> iosWriteRequestFail;
   be2_val<uint32_t> iosWriteRequestSuccess;
   be2_val<uint32_t> iosWriteAsyncRequestSubmitFail;
   be2_val<uint32_t> iosWriteAsyncRequestSubmitSuccess;
   be2_val<uint32_t> iosWriteAsyncRequestFail;
   be2_val<uint32_t> iosWriteAsyncRequestSuccess;
   be2_val<uint32_t> iosWriteAsyncExRequestSubmitFail;
   be2_val<uint32_t> iosWriteAsyncExRequestSubmitSuccess;
   be2_val<uint32_t> iosWriteAsyncExRequestFail;
   be2_val<uint32_t> iosWriteAsyncExRequestSuccess;
   be2_val<uint32_t> iosSeekRequestFail;
   be2_val<uint32_t> iosSeekRequestSuccess;
   be2_val<uint32_t> iosSeekAsyncRequestSubmitFail;
   be2_val<uint32_t> iosSeekAsyncRequestSubmitSuccess;
   be2_val<uint32_t> iosSeekAsyncRequestFail;
   be2_val<uint32_t> iosSeekAsyncRequestSuccess;
   be2_val<uint32_t> iosSeekAsyncExRequestSubmitFail;
   be2_val<uint32_t> iosSeekAsyncExRequestSubmitSuccess;
   be2_val<uint32_t> iosSeekAsyncExRequestFail;
   be2_val<uint32_t> iosSeekAsyncExRequestSuccess;
   be2_val<uint32_t> iosIoctlRequestFail;
   be2_val<uint32_t> iosIoctlRequestSuccess;
   be2_val<uint32_t> iosIoctlAsyncRequestSubmitFail;
   be2_val<uint32_t> iosIoctlAsyncRequestSubmitSuccess;
   be2_val<uint32_t> iosIoctlAsyncRequestFail;
   be2_val<uint32_t> iosIoctlAsyncRequestSuccess;
   be2_val<uint32_t> iosIoctlAsyncExRequestSubmitFail;
   be2_val<uint32_t> iosIoctlAsyncExRequestSubmitSuccess;
   be2_val<uint32_t> iosIoctlAsyncExRequestFail;
   be2_val<uint32_t> iosIoctlAsyncExRequestSuccess;
   be2_val<uint32_t> iosIoctlvRequestFail;
   be2_val<uint32_t> iosIoctlvRequestSuccess;
   be2_val<uint32_t> iosIoctlvAsyncRequestSubmitFail;
   be2_val<uint32_t> iosIoctlvAsyncRequestSubmitSuccess;
   be2_val<uint32_t> iosIoctlvAsyncRequestFail;
   be2_val<uint32_t> iosIoctlvAsyncRequestSuccess;
   be2_val<uint32_t> iosIoctlvAsyncExRequestSubmitFail;
   be2_val<uint32_t> iosIoctlvAsyncExRequestSubmitSuccess;
   be2_val<uint32_t> iosIoctlvAsyncExRequestFail;
   be2_val<uint32_t> iosIoctlvAsyncExRequestSuccess;
   be2_val<uint32_t> requestsProcessed;
   be2_val<uint32_t> requestsSubmitted;
   be2_val<uint32_t> repliesReceived;
   be2_val<uint32_t> asyncTransactionsCompleted;
   UNKNOWN(4);
   be2_val<uint32_t> syncTransactionsCompleted;
   be2_val<uint32_t> invalidReplyAddress;
   be2_val<uint32_t> unexpectedReplyInterrupt;
   be2_val<uint32_t> unexpectedAckInterrupt;
   be2_val<uint32_t> invalidReplyMessagePointer;
   be2_val<uint32_t> invalidReplyMessagePointerNotAlloc;
   be2_val<uint32_t> invalidReplyCommand;
   be2_val<uint32_t> failedAllocateRequestBlock;
   be2_val<uint32_t> failedFreeRequestBlock;
   be2_val<uint32_t> failedRequestSubmitOutboundFIFOFull;

   //! FIFO of free IPCDriverRequests.
   be2_struct<IPCDriverFIFO> freeFifo;

   //! FIFO of IPCDriverRequests which have been sent over IPC and are awaiting a reply.
   be2_struct<IPCDriverFIFO> outboundFifo;

   //! An event object used to wait for a request to be available for allocation from freeFifo.
   be2_struct<OSEvent> waitFreeFifoEvent;

   //! Set to TRUE if there is a someone waiting on waitFreeFifoEvent.
   be2_val<BOOL> waitingFreeFifo;

   //! Set to TRUE once this->requests has been initialised.
   be2_val<BOOL> initialisedRequests;

   //! Number of pending responses from IPC to process.
   be2_val<uint32_t> numResponses;

   //! List of pending responses from IPC to process.
   be2_array<virt_ptr<IPCKDriverRequest>, IPCBufferCount> responses;

   //! IPCDriverRequests memory to be used in freeFifo / outboundFifo.
   be2_array<IPCDriverRequest, IPCBufferCount> requests;

   UNKNOWN(0x1740 - 0xF3C);
};
CHECK_OFFSET(IPCDriver, 0x00, status);
CHECK_OFFSET(IPCDriver, 0x08, coreId);
CHECK_OFFSET(IPCDriver, 0x10, ipcBuffers);
CHECK_OFFSET(IPCDriver, 0x14, currentSendTransaction);
CHECK_OFFSET(IPCDriver, 0x18, iosOpenRequestFail);
CHECK_OFFSET(IPCDriver, 0x1C, iosOpenRequestSuccess);
CHECK_OFFSET(IPCDriver, 0x20, iosOpenAsyncRequestSubmitFail);
CHECK_OFFSET(IPCDriver, 0x24, iosOpenAsyncRequestSubmitSuccess);
CHECK_OFFSET(IPCDriver, 0x28, iosOpenAsyncRequestFail);
CHECK_OFFSET(IPCDriver, 0x2C, iosOpenAsyncRequestSuccess);
CHECK_OFFSET(IPCDriver, 0x30, iosOpenAsyncExRequestSubmitFail);
CHECK_OFFSET(IPCDriver, 0x34, iosOpenAsyncExRequestSubmitSuccess);
CHECK_OFFSET(IPCDriver, 0x38, iosOpenAsyncExRequestFail);
CHECK_OFFSET(IPCDriver, 0x3C, iosOpenAsyncExRequestSuccess);
CHECK_OFFSET(IPCDriver, 0x40, iosCloseRequestFail);
CHECK_OFFSET(IPCDriver, 0x44, iosCloseRequestSuccess);
CHECK_OFFSET(IPCDriver, 0x48, iosCloseAsyncRequestSubmitFail);
CHECK_OFFSET(IPCDriver, 0x4C, iosCloseAsyncRequestSubmitSuccess);
CHECK_OFFSET(IPCDriver, 0x50, iosCloseAsyncRequestFail);
CHECK_OFFSET(IPCDriver, 0x54, iosCloseAsyncRequestSuccess);
CHECK_OFFSET(IPCDriver, 0x58, iosCloseAsyncExRequestSubmitFail);
CHECK_OFFSET(IPCDriver, 0x5C, iosCloseAsyncExRequestSubmitSuccess);
CHECK_OFFSET(IPCDriver, 0x60, iosCloseAsyncExRequestFail);
CHECK_OFFSET(IPCDriver, 0x64, iosCloseAsyncExRequestSuccess);
CHECK_OFFSET(IPCDriver, 0x68, iosReadRequestFail);
CHECK_OFFSET(IPCDriver, 0x6C, iosReadRequestSuccess);
CHECK_OFFSET(IPCDriver, 0x70, iosReadAsyncRequestSubmitFail);
CHECK_OFFSET(IPCDriver, 0x74, iosReadAsyncRequestSubmitSuccess);
CHECK_OFFSET(IPCDriver, 0x78, iosReadAsyncRequestFail);
CHECK_OFFSET(IPCDriver, 0x7C, iosReadAsyncRequestSuccess);
CHECK_OFFSET(IPCDriver, 0x80, iosReadAsyncExRequestSubmitFail);
CHECK_OFFSET(IPCDriver, 0x84, iosReadAsyncExRequestSubmitSuccess);
CHECK_OFFSET(IPCDriver, 0x88, iosReadAsyncExRequestFail);
CHECK_OFFSET(IPCDriver, 0x8C, iosReadAsyncExRequestSuccess);
CHECK_OFFSET(IPCDriver, 0x90, iosWriteRequestFail);
CHECK_OFFSET(IPCDriver, 0x94, iosWriteRequestSuccess);
CHECK_OFFSET(IPCDriver, 0x98, iosWriteAsyncRequestSubmitFail);
CHECK_OFFSET(IPCDriver, 0x9C, iosWriteAsyncRequestSubmitSuccess);
CHECK_OFFSET(IPCDriver, 0xA0, iosWriteAsyncRequestFail);
CHECK_OFFSET(IPCDriver, 0xA4, iosWriteAsyncRequestSuccess);
CHECK_OFFSET(IPCDriver, 0xA8, iosWriteAsyncExRequestSubmitFail);
CHECK_OFFSET(IPCDriver, 0xAC, iosWriteAsyncExRequestSubmitSuccess);
CHECK_OFFSET(IPCDriver, 0xB0, iosWriteAsyncExRequestFail);
CHECK_OFFSET(IPCDriver, 0xB4, iosWriteAsyncExRequestSuccess);
CHECK_OFFSET(IPCDriver, 0xB8, iosSeekRequestFail);
CHECK_OFFSET(IPCDriver, 0xBC, iosSeekRequestSuccess);
CHECK_OFFSET(IPCDriver, 0xC0, iosSeekAsyncRequestSubmitFail);
CHECK_OFFSET(IPCDriver, 0xC4, iosSeekAsyncRequestSubmitSuccess);
CHECK_OFFSET(IPCDriver, 0xC8, iosSeekAsyncRequestFail);
CHECK_OFFSET(IPCDriver, 0xCC, iosSeekAsyncRequestSuccess);
CHECK_OFFSET(IPCDriver, 0xD0, iosSeekAsyncExRequestSubmitFail);
CHECK_OFFSET(IPCDriver, 0xD4, iosSeekAsyncExRequestSubmitSuccess);
CHECK_OFFSET(IPCDriver, 0xD8, iosSeekAsyncExRequestFail);
CHECK_OFFSET(IPCDriver, 0xDC, iosSeekAsyncExRequestSuccess);
CHECK_OFFSET(IPCDriver, 0xE0, iosIoctlRequestFail);
CHECK_OFFSET(IPCDriver, 0xE4, iosIoctlRequestSuccess);
CHECK_OFFSET(IPCDriver, 0xE8, iosIoctlAsyncRequestSubmitFail);
CHECK_OFFSET(IPCDriver, 0xEC, iosIoctlAsyncRequestSubmitSuccess);
CHECK_OFFSET(IPCDriver, 0xF0, iosIoctlAsyncRequestFail);
CHECK_OFFSET(IPCDriver, 0xF4, iosIoctlAsyncRequestSuccess);
CHECK_OFFSET(IPCDriver, 0xF8, iosIoctlAsyncExRequestSubmitFail);
CHECK_OFFSET(IPCDriver, 0xFC, iosIoctlAsyncExRequestSubmitSuccess);
CHECK_OFFSET(IPCDriver, 0x100, iosIoctlAsyncExRequestFail);
CHECK_OFFSET(IPCDriver, 0x104, iosIoctlAsyncExRequestSuccess);
CHECK_OFFSET(IPCDriver, 0x108, iosIoctlvRequestFail);
CHECK_OFFSET(IPCDriver, 0x10C, iosIoctlvRequestSuccess);
CHECK_OFFSET(IPCDriver, 0x110, iosIoctlvAsyncRequestSubmitFail);
CHECK_OFFSET(IPCDriver, 0x114, iosIoctlvAsyncRequestSubmitSuccess);
CHECK_OFFSET(IPCDriver, 0x118, iosIoctlvAsyncRequestFail);
CHECK_OFFSET(IPCDriver, 0x11C, iosIoctlvAsyncRequestSuccess);
CHECK_OFFSET(IPCDriver, 0x120, iosIoctlvAsyncExRequestSubmitFail);
CHECK_OFFSET(IPCDriver, 0x124, iosIoctlvAsyncExRequestSubmitSuccess);
CHECK_OFFSET(IPCDriver, 0x128, iosIoctlvAsyncExRequestFail);
CHECK_OFFSET(IPCDriver, 0x12C, iosIoctlvAsyncExRequestSuccess);
CHECK_OFFSET(IPCDriver, 0x130, requestsProcessed);
CHECK_OFFSET(IPCDriver, 0x134, requestsSubmitted);
CHECK_OFFSET(IPCDriver, 0x138, repliesReceived);
CHECK_OFFSET(IPCDriver, 0x13C, asyncTransactionsCompleted);
CHECK_OFFSET(IPCDriver, 0x144, syncTransactionsCompleted);
CHECK_OFFSET(IPCDriver, 0x148, invalidReplyAddress);
CHECK_OFFSET(IPCDriver, 0x14C, unexpectedReplyInterrupt);
CHECK_OFFSET(IPCDriver, 0x150, unexpectedAckInterrupt);
CHECK_OFFSET(IPCDriver, 0x154, invalidReplyMessagePointer);
CHECK_OFFSET(IPCDriver, 0x158, invalidReplyMessagePointerNotAlloc);
CHECK_OFFSET(IPCDriver, 0x15C, invalidReplyCommand);
CHECK_OFFSET(IPCDriver, 0x160, failedAllocateRequestBlock);
CHECK_OFFSET(IPCDriver, 0x164, failedFreeRequestBlock);
CHECK_OFFSET(IPCDriver, 0x168, failedRequestSubmitOutboundFIFOFull);
CHECK_OFFSET(IPCDriver, 0x16C, freeFifo);
CHECK_OFFSET(IPCDriver, 0x23C, outboundFifo);
CHECK_OFFSET(IPCDriver, 0x30C, waitFreeFifoEvent);
CHECK_OFFSET(IPCDriver, 0x330, waitingFreeFifo);
CHECK_OFFSET(IPCDriver, 0x334, initialisedRequests);
CHECK_OFFSET(IPCDriver, 0x338, numResponses);
CHECK_OFFSET(IPCDriver, 0x33C, responses);
CHECK_OFFSET(IPCDriver, 0x3FC, requests);
CHECK_SIZE(IPCDriver, 0x1740);

void
IPCDriverInit();

IOSError
IPCDriverOpen();

IOSError
IPCDriverClose();

namespace internal
{

virt_ptr<IPCDriver>
getIPCDriver();

void
ipcDriverFifoInit(virt_ptr<IPCDriverFIFO> fifo);

IOSError
ipcDriverFifoPush(virt_ptr<IPCDriverFIFO> fifo,
                  virt_ptr<IPCDriverRequest> request);

IOSError
ipcDriverFifoPop(virt_ptr<IPCDriverFIFO> fifo,
                 virt_ptr<IPCDriverRequest> *outRequest);

IOSError
ipcDriverAllocateRequest(virt_ptr<IPCDriver> driver,
                         virt_ptr<IPCDriverRequest> *outRequest,
                         IOSHandle handle,
                         IOSCommand command,
                         uint32_t requestUnk0x04,
                         IOSAsyncCallbackFn asyncCallback,
                         virt_ptr<void> asyncContext);

IOSError
ipcDriverFreeRequest(virt_ptr<IPCDriver> driver,
                     virt_ptr<IPCDriverRequest> request);

IOSError
ipcDriverSubmitRequest(virt_ptr<IPCDriver> driver,
                       virt_ptr<IPCDriverRequest> request);

IOSError
ipcDriverWaitResponse(virt_ptr<IPCDriver> driver,
                      virt_ptr<IPCDriverRequest> request);

void
ipcDriverProcessResponses();

} // namespace internal

/** @} */

} // namespace cafe::coreinit

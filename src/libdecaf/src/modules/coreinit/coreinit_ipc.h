#pragma once
#include "coreinit_enum.h"
#include "coreinit_event.h"
#include "coreinit_ios.h"
#include "kernel/kernel_ipc.h"

#include <cstdint>
#include <common/be_ptr.h>
#include <common/be_val.h>
#include <common/structsize.h>
#include <libcpu/be2_struct.h>

namespace coreinit
{

/**
 * \defgroup coreinit_ipc IPC
 * \ingroup coreinit
 * @{
 */

constexpr size_t
IPCBufferCount = 0x30;

using IPCBuffer = kernel::IpcRequest;

/**
 * Contains all data required for IPCDriver about an IPC request.
 *
 * ipcBuffer contains the actual data sent over IPC.
 */
struct IPCDriverRequest
{
   //! Whether the current request has been allocated.
   be_val<BOOL> allocated;

   //! Unknown.
   be_val<uint32_t> unk0x04;

   //! Callback to be called when reply is received.
   IOSAsyncCallbackFn::be asyncCallback;

   //! Context for asyncCallback.
   be_ptr<void> asyncContext;

   UNKNOWN(0x4);

   //! Pointer to the actual IPC buffer.
   be_ptr<IPCBuffer> ipcBuffer;

   //! OSEvent called when request is finished.
   OSEvent finishEvent;
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
   be_val<int32_t> pushIndex;

   //! The current item index to pop from
   be_val<int32_t> popIndex;

   //! The number of items in the queue
   be_val<int32_t> count;

   //! Tracks the highest amount of items there has been in the queue
   be_val<int32_t> maxCount;

   //! Items in the queue
   be_ptr<IPCDriverRequest> requests[IPCBufferCount];
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
   be_val<IPCDriverStatus> status;

   UNKNOWN(0x4);

   //! The core this driver was opened on.
   be_val<uint32_t> coreId;

   UNKNOWN(0x4);

   //! A pointer to the memory used for IPCBuffers.
   be_ptr<IPCBuffer> ipcBuffers;

   //! The current outgoing IPCDriverRequest.
   be_ptr<IPCDriverRequest> currentSendTransaction;

   be_val<uint32_t> iosOpenRequestFail;
   be_val<uint32_t> iosOpenRequestSuccess;
   be_val<uint32_t> iosOpenAsyncRequestSubmitFail;
   be_val<uint32_t> iosOpenAsyncRequestSubmitSuccess;
   be_val<uint32_t> iosOpenAsyncRequestFail;
   be_val<uint32_t> iosOpenAsyncRequestSuccess;
   be_val<uint32_t> iosOpenAsyncExRequestSubmitFail;
   be_val<uint32_t> iosOpenAsyncExRequestSubmitSuccess;
   be_val<uint32_t> iosOpenAsyncExRequestFail;
   be_val<uint32_t> iosOpenAsyncExRequestSuccess;
   be_val<uint32_t> iosCloseRequestFail;
   be_val<uint32_t> iosCloseRequestSuccess;
   be_val<uint32_t> iosCloseAsyncRequestSubmitFail;
   be_val<uint32_t> iosCloseAsyncRequestSubmitSuccess;
   be_val<uint32_t> iosCloseAsyncRequestFail;
   be_val<uint32_t> iosCloseAsyncRequestSuccess;
   be_val<uint32_t> iosCloseAsyncExRequestSubmitFail;
   be_val<uint32_t> iosCloseAsyncExRequestSubmitSuccess;
   be_val<uint32_t> iosCloseAsyncExRequestFail;
   be_val<uint32_t> iosCloseAsyncExRequestSuccess;
   be_val<uint32_t> iosReadRequestFail;
   be_val<uint32_t> iosReadRequestSuccess;
   be_val<uint32_t> iosReadAsyncRequestSubmitFail;
   be_val<uint32_t> iosReadAsyncRequestSubmitSuccess;
   be_val<uint32_t> iosReadAsyncRequestFail;
   be_val<uint32_t> iosReadAsyncRequestSuccess;
   be_val<uint32_t> iosReadAsyncExRequestSubmitFail;
   be_val<uint32_t> iosReadAsyncExRequestSubmitSuccess;
   be_val<uint32_t> iosReadAsyncExRequestFail;
   be_val<uint32_t> iosReadAsyncExRequestSuccess;
   be_val<uint32_t> iosWriteRequestFail;
   be_val<uint32_t> iosWriteRequestSuccess;
   be_val<uint32_t> iosWriteAsyncRequestSubmitFail;
   be_val<uint32_t> iosWriteAsyncRequestSubmitSuccess;
   be_val<uint32_t> iosWriteAsyncRequestFail;
   be_val<uint32_t> iosWriteAsyncRequestSuccess;
   be_val<uint32_t> iosWriteAsyncExRequestSubmitFail;
   be_val<uint32_t> iosWriteAsyncExRequestSubmitSuccess;
   be_val<uint32_t> iosWriteAsyncExRequestFail;
   be_val<uint32_t> iosWriteAsyncExRequestSuccess;
   be_val<uint32_t> iosSeekRequestFail;
   be_val<uint32_t> iosSeekRequestSuccess;
   be_val<uint32_t> iosSeekAsyncRequestSubmitFail;
   be_val<uint32_t> iosSeekAsyncRequestSubmitSuccess;
   be_val<uint32_t> iosSeekAsyncRequestFail;
   be_val<uint32_t> iosSeekAsyncRequestSuccess;
   be_val<uint32_t> iosSeekAsyncExRequestSubmitFail;
   be_val<uint32_t> iosSeekAsyncExRequestSubmitSuccess;
   be_val<uint32_t> iosSeekAsyncExRequestFail;
   be_val<uint32_t> iosSeekAsyncExRequestSuccess;
   be_val<uint32_t> iosIoctlRequestFail;
   be_val<uint32_t> iosIoctlRequestSuccess;
   be_val<uint32_t> iosIoctlAsyncRequestSubmitFail;
   be_val<uint32_t> iosIoctlAsyncRequestSubmitSuccess;
   be_val<uint32_t> iosIoctlAsyncRequestFail;
   be_val<uint32_t> iosIoctlAsyncRequestSuccess;
   be_val<uint32_t> iosIoctlAsyncExRequestSubmitFail;
   be_val<uint32_t> iosIoctlAsyncExRequestSubmitSuccess;
   be_val<uint32_t> iosIoctlAsyncExRequestFail;
   be_val<uint32_t> iosIoctlAsyncExRequestSuccess;
   be_val<uint32_t> iosIoctlvRequestFail;
   be_val<uint32_t> iosIoctlvRequestSuccess;
   be_val<uint32_t> iosIoctlvAsyncRequestSubmitFail;
   be_val<uint32_t> iosIoctlvAsyncRequestSubmitSuccess;
   be_val<uint32_t> iosIoctlvAsyncRequestFail;
   be_val<uint32_t> iosIoctlvAsyncRequestSuccess;
   be_val<uint32_t> iosIoctlvAsyncExRequestSubmitFail;
   be_val<uint32_t> iosIoctlvAsyncExRequestSubmitSuccess;
   be_val<uint32_t> iosIoctlvAsyncExRequestFail;
   be_val<uint32_t> iosIoctlvAsyncExRequestSuccess;
   be_val<uint32_t> requestsProcessed;
   be_val<uint32_t> requestsSubmitted;
   be_val<uint32_t> repliesReceived;
   be_val<uint32_t> asyncTransactionsCompleted;
   UNKNOWN(4);
   be_val<uint32_t> syncTransactionsCompleted;
   be_val<uint32_t> invalidReplyAddress;
   be_val<uint32_t> unexpectedReplyInterrupt;
   be_val<uint32_t> unexpectedAckInterrupt;
   be_val<uint32_t> invalidReplyMessagePointer;
   be_val<uint32_t> invalidReplyMessagePointerNotAlloc;
   be_val<uint32_t> invalidReplyCommand;
   be_val<uint32_t> failedAllocateRequestBlock;
   be_val<uint32_t> failedFreeRequestBlock;
   UNKNOWN(4);

   //! FIFO of free IPCDriverRequests.
   IPCDriverFIFO freeFifo;

   //! FIFO of IPCDriverRequests which have been sent over IPC and are awaiting a reply.
   IPCDriverFIFO outboundFifo;

   //! An event object used to wait for a request to be available for allocation from freeFifo.
   OSEvent waitFreeFifoEvent;

   //! Set to TRUE if there is a someone waiting on waitFreeFifoEvent.
   be_val<BOOL> waitingFreeFifo;

   //! Set to TRUE once this->requests has been initialised.
   be_val<BOOL> initialisedRequests;

   //! Number of pending responses from IPC to process.
   be_val<uint32_t> numResponses;

   //! List of pending responses from IPC to process.
   be_ptr<IPCBuffer> responses[IPCBufferCount];

   //! IPCDriverRequests memory to be used in freeFifo / outboundFifo.
   IPCDriverRequest requests[IPCBufferCount];

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

IPCDriver *
getIPCDriver();

void
ipcDriverFifoInit(IPCDriverFIFO *fifo);

IOSError
ipcDriverFifoPush(IPCDriverFIFO *fifo,
                  IPCDriverRequest *request);

IOSError
ipcDriverFifoPop(IPCDriverFIFO *fifo,
                 IPCDriverRequest **requestOut);

IOSError
ipcDriverAllocateRequest(IPCDriver *driver,
                         IPCDriverRequest **requestOut,
                         IOSHandle handle,
                         IOSCommand command,
                         uint32_t requestUnk0x04,
                         IOSAsyncCallbackFn asyncCallback,
                         void *asyncContext);

IOSError
ipcDriverFreeRequest(IPCDriver *driver,
                     IPCDriverRequest *request);

IOSError
ipcDriverSubmitRequest(IPCDriver *ipcDriver,
                       IPCDriverRequest *ipcRequest);

IOSError
ipcDriverWaitResponse(IPCDriver *ipcDriver,
                      IPCDriverRequest *ipcRequest);

void
ipcDriverProcessResponses();

} // namespace internal

/** @} */

} // namespace coreinit

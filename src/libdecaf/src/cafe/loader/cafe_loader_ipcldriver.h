#pragma once
#include "cafe_loader_ipcldriverfifo.h"

#include "cafe/kernel/cafe_kernel_ipckdriver.h"
#include "ios/ios_enum.h"
#include "ios/ios_ipc.h"

#include <cstdint>
#include <common/cbool.h>
#include <functional>
#include <libcpu/be2_struct.h>

namespace cafe::loader::internal
{

#ifndef DECAF_LOADER_LLE
using IPCLAsyncCallbackFn = void(*)(ios::Error, virt_ptr<void>);
#endif

#pragma pack(push, 1)

constexpr auto IPCLBufferCount = 0x4u;

enum class IPCLDriverStatus : uint32_t
{
   Invalid = 0,
   Closed = 1,
   Initialised = 2,
   Open = 3,
   Submitting = 4,
};

struct IPCLDriverRequest
{
   be2_val<BOOL> allocated;
#ifdef DECAF_LOADER_LLE
   be2_val<uint32_t> asyncCallback;
   be2_virt_ptr<void> asyncCallbackData;
   UNKNOWN(4);
#else
   IPCLAsyncCallbackFn asyncCallback;
   be2_virt_ptr<void> asyncCallbackData;
#endif
   be2_virt_ptr<cafe::kernel::IPCKDriverRequest> ipckRequestBuffer;
};
CHECK_OFFSET(IPCLDriverRequest, 0x00, allocated);
CHECK_OFFSET(IPCLDriverRequest, 0x04, asyncCallback);
#ifdef DECAF_LOADER_LLE
CHECK_OFFSET(IPCLDriverRequest, 0x08, asyncCallbackData);
#endif
CHECK_OFFSET(IPCLDriverRequest, 0x10, ipckRequestBuffer);
CHECK_SIZE(IPCLDriverRequest, 0x14);

struct IPCLDriver
{
   be2_val<IPCLDriverStatus> status;
   UNKNOWN(0x4);
   be2_val<uint32_t> coreId;
   be2_virt_ptr<cafe::kernel::IPCKDriverRequest> ipckRequestBuffer;
   be2_virt_ptr<IPCLDriverRequest> currentSendTransaction;
   be2_val<uint32_t> iosOpenRequestFail;
   be2_val<uint32_t> iosOpenRequestSuccess;
   be2_val<uint32_t> iosOpenAsyncRequestSubmitFail;
   be2_val<uint32_t> iosOpenAsyncRequestSubmitSuccess;
   be2_val<uint32_t> iosOpenAsyncRequestFail;
   be2_val<uint32_t> iosOpenAsyncRequestSuccess;
   be2_val<uint32_t> iosCloseRequestFail;
   be2_val<uint32_t> iosCloseRequestSuccess;
   be2_val<uint32_t> iosCloseAsyncRequestSubmitFail;
   be2_val<uint32_t> iosCloseAsyncRequestSubmitSuccess;
   be2_val<uint32_t> iosCloseAsyncRequestFail;
   be2_val<uint32_t> iosCloseAsyncRequestSuccess;
   be2_val<uint32_t> iosReadRequestFail;
   be2_val<uint32_t> iosReadRequestSuccess;
   be2_val<uint32_t> iosReadAsyncRequestSubmitFail;
   be2_val<uint32_t> iosReadAsyncRequestSubmitSuccess;
   be2_val<uint32_t> iosReadAsyncRequestFail;
   be2_val<uint32_t> iosReadAsyncRequestSuccess;
   be2_val<uint32_t> iosWriteRequestFail;
   be2_val<uint32_t> iosWriteRequestSuccess;
   be2_val<uint32_t> iosWriteAsyncRequestSubmitFail;
   be2_val<uint32_t> iosWriteAsyncRequestSubmitSuccess;
   be2_val<uint32_t> iosWriteAsyncRequestFail;
   be2_val<uint32_t> iosWriteAsyncRequestSuccess;
   be2_val<uint32_t> iosSeekRequestFail;
   be2_val<uint32_t> iosSeekRequestSuccess;
   be2_val<uint32_t> iosSeekAsyncRequestSubmitFail;
   be2_val<uint32_t> iosSeekAsyncRequestSubmitSuccess;
   be2_val<uint32_t> iosSeekAsyncRequestFail;
   be2_val<uint32_t> iosSeekAsyncRequestSuccess;
   be2_val<uint32_t> iosIoctlRequestFail;
   be2_val<uint32_t> iosIoctlRequestSuccess;
   be2_val<uint32_t> iosIoctlAsyncRequestSubmitFail;
   be2_val<uint32_t> iosIoctlAsyncRequestSubmitSuccess;
   be2_val<uint32_t> iosIoctlAsyncRequestFail;
   be2_val<uint32_t> iosIoctlAsyncRequestSuccess;
   be2_val<uint32_t> iosIoctlvRequestFail;
   be2_val<uint32_t> iosIoctlvRequestSuccess;
   be2_val<uint32_t> iosIoctlvAsyncRequestSubmitFail;
   be2_val<uint32_t> iosIoctlvAsyncRequestSubmitSuccess;
   be2_val<uint32_t> iosIoctlvAsyncRequestFail;
   be2_val<uint32_t> iosIoctlvAsyncRequestSuccess;
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
   be2_struct<IPCLDriverFIFO<IPCLBufferCount>> freeFifo;
   be2_struct<IPCLDriverFIFO<IPCLBufferCount>> outboundFifo;
   be2_array<IPCLDriverRequest, IPCLBufferCount> requests;
};
CHECK_OFFSET(IPCLDriver, 0x00, status);
CHECK_OFFSET(IPCLDriver, 0x08, coreId);
CHECK_OFFSET(IPCLDriver, 0x0C, ipckRequestBuffer);
CHECK_OFFSET(IPCLDriver, 0x10, currentSendTransaction);
CHECK_OFFSET(IPCLDriver, 0x14, iosOpenRequestFail);
CHECK_OFFSET(IPCLDriver, 0x18, iosOpenRequestSuccess);
CHECK_OFFSET(IPCLDriver, 0x1C, iosOpenAsyncRequestSubmitFail);
CHECK_OFFSET(IPCLDriver, 0x20, iosOpenAsyncRequestSubmitSuccess);
CHECK_OFFSET(IPCLDriver, 0x24, iosOpenAsyncRequestFail);
CHECK_OFFSET(IPCLDriver, 0x28, iosOpenAsyncRequestSuccess);
CHECK_OFFSET(IPCLDriver, 0x2C, iosCloseRequestFail);
CHECK_OFFSET(IPCLDriver, 0x30, iosCloseRequestSuccess);
CHECK_OFFSET(IPCLDriver, 0x34, iosCloseAsyncRequestSubmitFail);
CHECK_OFFSET(IPCLDriver, 0x38, iosCloseAsyncRequestSubmitSuccess);
CHECK_OFFSET(IPCLDriver, 0x3C, iosCloseAsyncRequestFail);
CHECK_OFFSET(IPCLDriver, 0x40, iosCloseAsyncRequestSuccess);
CHECK_OFFSET(IPCLDriver, 0x44, iosReadRequestFail);
CHECK_OFFSET(IPCLDriver, 0x48, iosReadRequestSuccess);
CHECK_OFFSET(IPCLDriver, 0x4C, iosReadAsyncRequestSubmitFail);
CHECK_OFFSET(IPCLDriver, 0x50, iosReadAsyncRequestSubmitSuccess);
CHECK_OFFSET(IPCLDriver, 0x54, iosReadAsyncRequestFail);
CHECK_OFFSET(IPCLDriver, 0x58, iosReadAsyncRequestSuccess);
CHECK_OFFSET(IPCLDriver, 0x5C, iosWriteRequestFail);
CHECK_OFFSET(IPCLDriver, 0x60, iosWriteRequestSuccess);
CHECK_OFFSET(IPCLDriver, 0x64, iosWriteAsyncRequestSubmitFail);
CHECK_OFFSET(IPCLDriver, 0x68, iosWriteAsyncRequestSubmitSuccess);
CHECK_OFFSET(IPCLDriver, 0x6C, iosWriteAsyncRequestFail);
CHECK_OFFSET(IPCLDriver, 0x70, iosWriteAsyncRequestSuccess);
CHECK_OFFSET(IPCLDriver, 0x74, iosSeekRequestFail);
CHECK_OFFSET(IPCLDriver, 0x78, iosSeekRequestSuccess);
CHECK_OFFSET(IPCLDriver, 0x7C, iosSeekAsyncRequestSubmitFail);
CHECK_OFFSET(IPCLDriver, 0x80, iosSeekAsyncRequestSubmitSuccess);
CHECK_OFFSET(IPCLDriver, 0x84, iosSeekAsyncRequestFail);
CHECK_OFFSET(IPCLDriver, 0x88, iosSeekAsyncRequestSuccess);
CHECK_OFFSET(IPCLDriver, 0x8C, iosIoctlRequestFail);
CHECK_OFFSET(IPCLDriver, 0x90, iosIoctlRequestSuccess);
CHECK_OFFSET(IPCLDriver, 0x94, iosIoctlAsyncRequestSubmitFail);
CHECK_OFFSET(IPCLDriver, 0x98, iosIoctlAsyncRequestSubmitSuccess);
CHECK_OFFSET(IPCLDriver, 0x9C, iosIoctlAsyncRequestFail);
CHECK_OFFSET(IPCLDriver, 0xA0, iosIoctlAsyncRequestSuccess);
CHECK_OFFSET(IPCLDriver, 0xA4, iosIoctlvRequestFail);
CHECK_OFFSET(IPCLDriver, 0xA8, iosIoctlvRequestSuccess);
CHECK_OFFSET(IPCLDriver, 0xAC, iosIoctlvAsyncRequestSubmitFail);
CHECK_OFFSET(IPCLDriver, 0xB0, iosIoctlvAsyncRequestSubmitSuccess);
CHECK_OFFSET(IPCLDriver, 0xB4, iosIoctlvAsyncRequestFail);
CHECK_OFFSET(IPCLDriver, 0xB8, iosIoctlvAsyncRequestSuccess);
CHECK_OFFSET(IPCLDriver, 0xBC, requestsProcessed);
CHECK_OFFSET(IPCLDriver, 0xC0, requestsSubmitted);
CHECK_OFFSET(IPCLDriver, 0xC4, repliesReceived);
CHECK_OFFSET(IPCLDriver, 0xC8, asyncTransactionsCompleted);
CHECK_OFFSET(IPCLDriver, 0xD0, syncTransactionsCompleted);
CHECK_OFFSET(IPCLDriver, 0xD4, invalidReplyAddress);
CHECK_OFFSET(IPCLDriver, 0xD8, unexpectedReplyInterrupt);
CHECK_OFFSET(IPCLDriver, 0xDC, unexpectedAckInterrupt);
CHECK_OFFSET(IPCLDriver, 0xE0, invalidReplyMessagePointer);
CHECK_OFFSET(IPCLDriver, 0xE4, invalidReplyMessagePointerNotAlloc);
CHECK_OFFSET(IPCLDriver, 0xE8, invalidReplyCommand);
CHECK_OFFSET(IPCLDriver, 0xEC, failedAllocateRequestBlock);
CHECK_OFFSET(IPCLDriver, 0xF0, failedFreeRequestBlock);
CHECK_OFFSET(IPCLDriver, 0xF4, failedRequestSubmitOutboundFIFOFull);
CHECK_OFFSET(IPCLDriver, 0xF8, freeFifo);
CHECK_OFFSET(IPCLDriver, 0x118, outboundFifo);
CHECK_OFFSET(IPCLDriver, 0x138, requests);
CHECK_SIZE(IPCLDriver, 0x188);

#pragma pack(pop)

ios::Error
IPCLDriver_Init();

ios::Error
IPCLDriver_Open();

ios::Error
IPCLDriver_GetInstance(virt_ptr<IPCLDriver> *outDriver);

ios::Error
IPCLDriver_ProcessReply(virt_ptr<IPCLDriver> driver,
                        virt_ptr<cafe::kernel::IPCKDriverRequest> ipckRequest);

ios::Error
IPCLDriver_IoctlAsync(ios::Handle handle,
                      uint32_t command,
                      virt_ptr<const void> inputBuffer,
                      uint32_t inputLength,
                      virt_ptr<void> outputBuffer,
                      uint32_t outputLength,
                      IPCLAsyncCallbackFn callback,
                      virt_ptr<void> callbackContext);

void
initialiseIpclDriverStaticData();

} // namespace cafe::loader::internal

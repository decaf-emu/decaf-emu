#pragma once
#include "cafe_kernel_interrupts.h"
#include "cafe_kernel_ipckdriverfifo.h"
#include "cafe_kernel_processid.h"

#include "ios/ios_error.h"
#include "ios/ios_ipc.h"

#include <common/bitfield.h>
#include <libcpu/be2_struct.h>

namespace cafe::kernel
{

#pragma pack(push, 1)

using IpcRequest = ios::IpcRequest;
using IpcCommand = ios::Command;

struct IPCKDriverRequest
{
   //! Actual IPC request
   be2_struct<IpcRequest> request;

   //! Allegedly the previous IPC command
   be2_val<IpcCommand> prevCommand;

   //! Allegedly the previous IPC handle
   be2_val<int32_t> prevHandle;

   //! Buffer argument 1
   be2_virt_ptr<void> buffer1;

   //! Buffer argument 2
   be2_virt_ptr<void> buffer2;

   //! Buffer to copy device name to for IOS_Open
   be2_array<char, 0x20> nameBuffer;

   UNKNOWN(0x80 - 0x68);
};
CHECK_OFFSET(IPCKDriverRequest, 0x00, request);
CHECK_OFFSET(IPCKDriverRequest, 0x38, prevCommand);
CHECK_OFFSET(IPCKDriverRequest, 0x3C, prevHandle);
CHECK_OFFSET(IPCKDriverRequest, 0x40, buffer1);
CHECK_OFFSET(IPCKDriverRequest, 0x44, buffer2);
CHECK_OFFSET(IPCKDriverRequest, 0x48, nameBuffer);
CHECK_SIZE(IPCKDriverRequest, 0x80);

struct IPCKDriverReplyQueue
{
   static constexpr auto Size = 0x30u;
   be2_val<uint32_t> numReplies;
   be2_array<virt_ptr<IPCKDriverRequest>, Size> replies;
};
CHECK_OFFSET(IPCKDriverReplyQueue, 0x00, numReplies);
CHECK_OFFSET(IPCKDriverReplyQueue, 0x04, replies);
CHECK_SIZE(IPCKDriverReplyQueue, 0xC4);

#pragma pack(pop)

ios::Error
ipckDriverUserOpen(uint32_t numReplies,
                   virt_ptr<IPCKDriverReplyQueue> replyQueue,
                   InterruptHandlerFn handler);

ios::Error
ipckDriverUserClose();

ios::Error
ipckDriverUserSubmitRequest(virt_ptr<IPCKDriverRequest> request);

void
ipckDriverIosSubmitReply(phys_ptr<ios::IpcRequest> reply);

namespace internal
{


#pragma pack(push, 1)

constexpr auto IPCKRequestsPerCore = 0xB0u;
constexpr auto IPCKRequestsPerProcess = 0x30u;

enum class IPCKDriverState : uint32_t
{
   Invalid = 0,
   Unknown1 = 1,
   Initialised = 2,
   Open = 3,
   Submitting = 4,
};

struct IPCKDriverRegisters
{
   be2_virt_ptr<void> ppcMsg;
   be2_virt_ptr<void> ppcCtrl;
   be2_virt_ptr<void> armMsg;
   be2_virt_ptr<void> ahbLt;
   be2_val<uint32_t> unkMaybeFlags;
};

// This is a pointer to a host function pointer - we must allocate guest
// memory to store the host function pointer in.
using IPCKDriverHostAsyncCallbackFn =
   void(*)(ios::Error, virt_ptr<void>);

struct IPCKDriverHostAsyncCallback
{
   IPCKDriverHostAsyncCallbackFn func;
};

struct IPCKDriverRequestBlock
{
   enum RequestState
   {
      Unallocated = 0,
      Allocated = 1,
   };

   enum ReplyState
   {
      WaitReply = 0,
      ReceivedReply = 1,
   };

   BITFIELD(Flags, uint32_t)
      BITFIELD_ENTRY(10, 2, uint8_t, unk_0x0C00)
      BITFIELD_ENTRY(12, 2, ReplyState, replyState)
      BITFIELD_ENTRY(14, 2, RequestState, requestState)
      BITFIELD_ENTRY(16, 8, RamPartitionId, loaderProcessId)
      BITFIELD_ENTRY(24, 8, RamPartitionId, clientProcessId)
   BITFIELD_END

   be2_val<Flags> flags;

   //! Kernel request callback
   be2_virt_ptr<IPCKDriverHostAsyncCallback> asyncCallback;

   //! Data passed to kernel callback
   be2_virt_ptr<void> asyncCallbackData;

   //! User memory for a request
   be2_virt_ptr<IPCKDriverRequest> userRequest;

   //! Kernel memory for request, assigned from driver's requestsBuffer
   be2_virt_ptr<IPCKDriverRequest> request;
};
CHECK_OFFSET(IPCKDriverRequestBlock, 0x00, flags);
CHECK_OFFSET(IPCKDriverRequestBlock, 0x04, asyncCallback);
CHECK_OFFSET(IPCKDriverRequestBlock, 0x08, asyncCallbackData);
CHECK_OFFSET(IPCKDriverRequestBlock, 0x0C, userRequest);
CHECK_OFFSET(IPCKDriverRequestBlock, 0x10, request);
CHECK_SIZE(IPCKDriverRequestBlock, 0x14);

struct IPCKDriver
{
   be2_val<IPCKDriverState> state;
   be2_val<uint32_t> unk0x04;
   be2_val<uint32_t> coreId;
   be2_val<InterruptType> interruptType;
   be2_virt_ptr<IPCKDriverRequest> requestsBuffer;
   UNKNOWN(0x4);
   be2_array<virt_ptr<IPCKDriverReplyQueue>, NumRamPartitions> perProcessReplyQueue;
#ifdef DECAF_KERNEL_LLE
   be2_array<virt_ptr<void>, NumRamPartitions> perProcessCallbacks;
   be2_array<virt_ptr<void>, NumRamPartitions> perProcessCallbackStacks;
   be2_array<virt_ptr<Context>, NumRamPartitions> perProcessCallbackContexts;
#else
   std::array<InterruptHandlerFn, NumRamPartitions> perProcessCallbacks;
   PADDING(0x98 - (0x38 + sizeof(InterruptHandlerFn) * NumRamPartitions));
#endif
   be2_array<ios::Error, NumRamPartitions> perProcessLastError;
   UNKNOWN(0x4);
   be2_struct<IPCKDriverRegisters> registers;
   UNKNOWN(0x8);
   be2_array<uint32_t, NumRamPartitions> perProcessNumUserRequests;
   be2_array<uint32_t, NumRamPartitions> perProcessNumLoaderRequests;
   be2_struct<IPCKDriverFIFO<IPCKRequestsPerCore>> freeFifo;
   be2_struct<IPCKDriverFIFO<IPCKRequestsPerCore>> outboundFIFO;
   be2_array<IPCKDriverFIFO<IPCKRequestsPerProcess>, NumRamPartitions> perProcessUserReply;
   be2_array<IPCKDriverFIFO<IPCKRequestsPerProcess>, NumRamPartitions> perProcessLoaderReply;
   be2_array<IPCKDriverRequestBlock, IPCKRequestsPerCore> requestBlocks;
};
CHECK_OFFSET(IPCKDriver, 0x0, state);
CHECK_OFFSET(IPCKDriver, 0x4, unk0x04);
CHECK_OFFSET(IPCKDriver, 0x8, coreId);
CHECK_OFFSET(IPCKDriver, 0xC, interruptType);
CHECK_OFFSET(IPCKDriver, 0x10, requestsBuffer);
CHECK_OFFSET(IPCKDriver, 0x18, perProcessReplyQueue);
#ifdef DECAF_KERNEL_LLE
CHECK_OFFSET(IPCKDriver, 0x38, perProcessCallbacks);
CHECK_OFFSET(IPCKDriver, 0x58, perProcessCallbackStacks);
CHECK_OFFSET(IPCKDriver, 0x78, perProcessCallbackContexts);
#endif
CHECK_OFFSET(IPCKDriver, 0x98, perProcessLastError);
CHECK_OFFSET(IPCKDriver, 0xBC, registers);
CHECK_OFFSET(IPCKDriver, 0xD8, perProcessNumUserRequests);
CHECK_OFFSET(IPCKDriver, 0xF8, perProcessNumLoaderRequests);
CHECK_OFFSET(IPCKDriver, 0x118, freeFifo);
CHECK_OFFSET(IPCKDriver, 0x3E8, outboundFIFO);
CHECK_OFFSET(IPCKDriver, 0x6B8, perProcessUserReply);
CHECK_OFFSET(IPCKDriver, 0xD38, perProcessLoaderReply);
CHECK_OFFSET(IPCKDriver, 0x13B8, requestBlocks);
CHECK_SIZE(IPCKDriver, 0x2178);

#pragma pack(pop)

virt_ptr<IPCKDriver>
ipckDriverGetInstance();

ios::Error
ipckDriverAllocateRequestBlock(RamPartitionId clientProcessId,
                               RamPartitionId loaderProcessId,
                               virt_ptr<IPCKDriver> driver,
                               virt_ptr<IPCKDriverRequestBlock> *outRequestBlock,
                               ios::Handle handle,
                               ios::Command command,
                               IPCKDriverHostAsyncCallbackFn asyncCallback,
                               virt_ptr<void> asyncContext);

void
ipckDriverFreeRequestBlock(virt_ptr<IPCKDriver> driver,
                           virt_ptr<IPCKDriverRequestBlock> requestBlock);

ios::Error
ipckDriverSubmitRequest(virt_ptr<IPCKDriver> driver,
                        virt_ptr<IPCKDriverRequestBlock> requestBlock);

ios::Error
ipckDriverInit();

ios::Error
ipckDriverOpen();

void
initialiseStaticIpckDriverData();

} // namespace internal

} // namespace cafe::kernel

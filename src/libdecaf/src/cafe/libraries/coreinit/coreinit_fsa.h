#pragma once
#include "coreinit_enum.h"
#include "coreinit_event.h"
#include "coreinit_fs.h"
#include "coreinit_ios.h"

#include "ios/fs/ios_fs_fsa.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_fsa FSA
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

using FSAClientHandle = IOSHandle;
using FSAAttachInfo = ios::fs::FSAAttachInfo;
using FSABlockInfo = ios::fs::FSABlockInfo;
using FSACommand = ios::fs::FSACommand;
using FSADeviceInfo = ios::fs::FSADeviceInfo;
using FSAFileHandle = ios::fs::FSAFileHandle;
using FSAFileSystemInfo = ios::fs::FSAFileSystemInfo;
using FSAQueryInfoType = ios::fs::FSAQueryInfoType;
using FSAReadFlag = ios::fs::FSAReadFlag;
using FSARequest = ios::fs::FSARequest;
using FSAResponse = ios::fs::FSAResponse;
using FSAStat = ios::fs::FSAStat;
using FSAStatus = ios::fs::FSAStatus;
using FSAVolumeInfo = ios::fs::FSAVolumeInfo;
using FSAWriteFlag = ios::fs::FSAWriteFlag;

struct FSAShimBuffer;
struct OSMessage;
struct OSMessageQueue;

using FSAAsyncCallbackFn =
   virt_func_ptr<void (FSAStatus result,
                       FSACommand command,
                       virt_ptr<FSARequest> request,
                       virt_ptr<FSAResponse> response,
                       virt_ptr<void> userContext)>;

using FSAClientAttachAsyncCallbackFn =
   virt_func_ptr<void (FSAStatus result,
                       uint32_t responseWord0,
                       virt_ptr<FSAAttachInfo> attachInfo,
                       virt_ptr<void> userContext)>;

/**
 * Async data passed to an FSA*Async function.
 */
struct FSAAsyncData
{
   //! Callback to call when the command is complete.
   be2_val<FSAAsyncCallbackFn> userCallback;

   //! Callback context
   be2_virt_ptr<void> userContext;

   //! Queue to put a message on when command is complete.
   be2_virt_ptr<OSMessageQueue> ioMsgQueue;
};
CHECK_OFFSET(FSAAsyncData, 0x00, userCallback);
CHECK_OFFSET(FSAAsyncData, 0x04, userContext);
CHECK_OFFSET(FSAAsyncData, 0x08, ioMsgQueue);
CHECK_SIZE(FSAAsyncData, 0xC);

struct FSAAsyncResult
{
   //! Queue to put a message on when command is complete.
   be2_virt_ptr<OSMessageQueue> ioMsgQueue;

   //! Message used for ioMsgQueue.
   be2_struct<FSMessage> msg;

   //! Callback to call when the command is complete.
   be2_val<FSAAsyncCallbackFn> userCallback;

   //! Result.
   be2_val<FSAStatus> error;

   //! FSA command.
   be2_val<FSACommand> command;

   //! Pointer to allocated FSA IPC Request.
   be2_virt_ptr<FSARequest> request;

   //! Pointer to allocated FSA IPC Response.
   be2_virt_ptr<FSAResponse> response;

   //! Callback to call when the command is complete.
   be2_virt_ptr<void> userContext;
};
CHECK_OFFSET(FSAAsyncResult, 0x00, ioMsgQueue);
CHECK_OFFSET(FSAAsyncResult, 0x04, msg);
CHECK_OFFSET(FSAAsyncResult, 0x14, userCallback);
CHECK_OFFSET(FSAAsyncResult, 0x18, error);
CHECK_OFFSET(FSAAsyncResult, 0x1C, command);
CHECK_OFFSET(FSAAsyncResult, 0x20, request);
CHECK_OFFSET(FSAAsyncResult, 0x24, response);
CHECK_OFFSET(FSAAsyncResult, 0x28, userContext);
CHECK_SIZE(FSAAsyncResult, 0x2C);

struct FSAClientAttachAsyncData
{
   //! Callback to call when an attach has happened.
   be2_val<FSAClientAttachAsyncCallbackFn> userCallback;

   //! Callback context
   be2_virt_ptr<void> userContext;

   //! Queue to put a message on when command is complete.
   be2_virt_ptr<OSMessageQueue> ioMsgQueue;
};
CHECK_OFFSET(FSAClientAttachAsyncData, 0x00, userCallback);
CHECK_OFFSET(FSAClientAttachAsyncData, 0x04, userContext);
CHECK_OFFSET(FSAClientAttachAsyncData, 0x08, ioMsgQueue);
CHECK_SIZE(FSAClientAttachAsyncData, 0xC);

struct FSAClient
{
   be2_val<FSAClientState> state;
   be2_val<IOSHandle> fsaHandle;
   be2_struct<FSAClientAttachAsyncData> asyncAttachData;
   be2_struct<FSMessage> attachMessage;
   be2_val<FSAStatus> lastGetAttachStatus;
   be2_val<uint32_t> attachResponseWord0;
   be2_struct<FSAAttachInfo> attachInfo;
   be2_virt_ptr<FSAShimBuffer> attachShimBuffer;
   be2_struct<OSEvent> attachEvent;
};
CHECK_OFFSET(FSAClient, 0x00, state);
CHECK_OFFSET(FSAClient, 0x04, fsaHandle);
CHECK_OFFSET(FSAClient, 0x08, asyncAttachData);
CHECK_OFFSET(FSAClient, 0x14, attachMessage);
CHECK_OFFSET(FSAClient, 0x24, lastGetAttachStatus);
CHECK_OFFSET(FSAClient, 0x28, attachResponseWord0);
CHECK_OFFSET(FSAClient, 0x2C, attachInfo);
CHECK_OFFSET(FSAClient, 0x1E8, attachShimBuffer);
CHECK_OFFSET(FSAClient, 0x1EC, attachEvent);
CHECK_SIZE(FSAClient, 0x210);

#pragma pack(pop)

FSAStatus
FSAInit();

FSAStatus
FSAShutdown();

FSAStatus
FSAAddClient(virt_ptr<FSAClientAttachAsyncData> attachAsyncData);

FSAStatus
FSADelClient(FSAClientHandle handle);

virt_ptr<FSAAsyncResult>
FSAGetAsyncResult(virt_ptr<OSMessage> message);

virt_ptr<FSAClient>
FSAShimCheckClientHandle(FSAClientHandle handle);

FSAStatus
FSAShimAllocateBuffer(virt_ptr<virt_ptr<FSAShimBuffer>> outBuffer);

void
FSAShimFreeBuffer(virt_ptr<FSAShimBuffer> buffer);

namespace internal
{

void
fsaAsyncResultInit(virt_ptr<FSAAsyncResult> asyncResult,
                   virt_ptr<const FSAAsyncData> asyncData,
                   OSFunctionType func);

FSStatus
fsaDecodeFsaStatusToFsStatus(FSAStatus error);

} // namespace internal

/** @} */

} // namespace cafe::coreinit

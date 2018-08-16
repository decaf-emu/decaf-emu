#pragma once
#include "coreinit_enum.h"
#include "coreinit_fs.h"
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

using FSACommand = ios::fs::FSACommand;
using FSAFileSystemInfo = ios::fs::FSAFileSystemInfo;
using FSAQueryInfoType = ios::fs::FSAQueryInfoType;
using FSAReadFlag = ios::fs::FSAReadFlag;
using FSARequest = ios::fs::FSARequest;
using FSAResponse = ios::fs::FSAResponse;
using FSAStatus = ios::fs::FSAStatus;
using FSAVolumeInfo = ios::fs::FSAVolumeInfo;
using FSAWriteFlag = ios::fs::FSAWriteFlag;

struct OSMessage;
struct OSMessageQueue;

using FSAAsyncCallbackFn = virt_func_ptr<void(FSAStatus,
                                              FSACommand,
                                              virt_ptr<FSARequest>,
                                              virt_ptr<FSAResponse>,
                                              virt_ptr<void>)>;

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

#pragma pack(pop)

virt_ptr<FSAAsyncResult>
FSAGetAsyncResult(virt_ptr<OSMessage> message);

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

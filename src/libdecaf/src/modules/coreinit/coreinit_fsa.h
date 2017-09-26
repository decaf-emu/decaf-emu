#pragma once
#include "coreinit_enum.h"
#include "coreinit_fs.h"
#include "ios/fs/ios_fs_fsa.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/be_ptr.h>
#include <common/structsize.h>

namespace coreinit
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

using FSAAsyncCallbackFn = wfunc_ptr<void, FSAStatus, FSACommand, FSARequest *, FSAResponse *, void *>;

/**
 * Async data passed to an FSA*Async function.
 */
struct FSAAsyncData
{
   //! Callback to call when the command is complete.
   FSAAsyncCallbackFn::be userCallback;

   //! Callback context
   be_ptr<void> userContext;

   //! Queue to put a message on when command is complete.
   be_ptr<OSMessageQueue> ioMsgQueue;
};
CHECK_OFFSET(FSAAsyncData, 0x00, userCallback);
CHECK_OFFSET(FSAAsyncData, 0x04, userContext);
CHECK_OFFSET(FSAAsyncData, 0x08, ioMsgQueue);
CHECK_SIZE(FSAAsyncData, 0xC);

struct FSAAsyncResult
{
   //! Queue to put a message on when command is complete.
   be_ptr<OSMessageQueue> ioMsgQueue;

   //! Message used for ioMsgQueue.
   FSMessage msg;

   //! Callback to call when the command is complete.
   FSAAsyncCallbackFn::be userCallback;

   //! Result.
   be_val<FSAStatus> error;

   //! FSA command.
   be_val<FSACommand> command;

   //! Pointer to allocated FSA IPC Request.
   be_ptr<FSARequest> request;

   //! Pointer to allocated FSA IPC Response.
   be_ptr<FSAResponse> response;

   //! Callback to call when the command is complete.
   be_ptr<void> userContext;
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

FSAAsyncResult *
FSAGetAsyncResult(OSMessage *message);

namespace internal
{

void
fsaAsyncResultInit(FSAAsyncResult *asyncResult,
                   const FSAAsyncData *asyncData,
                   OSFunctionType func);

FSStatus
fsaDecodeFsaStatusToFsStatus(FSAStatus error);

} // namespace internal

/** @} */

} // namespace coreinit

#pragma once
#include "coreinit_enum.h"
#include "coreinit_fs.h"

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

struct FSARequest;
struct FSAResponse;
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

struct FSAVolumeInfo
{
   be_val<uint32_t> flags;
   be_val<FSAMediaState> mediaState;
   UNKNOWN(0x4);
   be_val<uint32_t> unk0x0C;
   be_val<uint32_t> unk0x10;
   be_val<int32_t> unk0x14;
   be_val<int32_t> unk0x18;
   UNKNOWN(0x10);
   char volumeLabel[128];
   char volumeId[128];
   char devicePath[16];
   char mountPath[128];
};
CHECK_OFFSET(FSAVolumeInfo, 0x00, flags);
CHECK_OFFSET(FSAVolumeInfo, 0x04, mediaState);
CHECK_OFFSET(FSAVolumeInfo, 0x0C, unk0x0C);
CHECK_OFFSET(FSAVolumeInfo, 0x10, unk0x10);
CHECK_OFFSET(FSAVolumeInfo, 0x14, unk0x14);
CHECK_OFFSET(FSAVolumeInfo, 0x18, unk0x18);
CHECK_OFFSET(FSAVolumeInfo, 0x2C, volumeLabel);
CHECK_OFFSET(FSAVolumeInfo, 0xAC, volumeId);
CHECK_OFFSET(FSAVolumeInfo, 0x12C, devicePath);
CHECK_OFFSET(FSAVolumeInfo, 0x13C, mountPath);
CHECK_SIZE(FSAVolumeInfo, 0x1BC);

#pragma pack(pop)

FSAAsyncResult *
FSAGetAsyncResult(OSMessage *message);

namespace internal
{

void
fsaAsyncResultInit(FSAAsyncResult *asyncResult,
                   const FSAAsyncData *asyncData,
                   OSFunctionType func);

} // namespace internal

/** @} */

} // namespace coreinit

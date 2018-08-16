#pragma once
#include "coreinit_enum.h"
#include "coreinit_messagequeue.h"
#include "ios/fs/ios_fs_fsa.h"

#include <libcpu/be2_struct.h>

/*
Unimplemented functions:
FSAppendFile
FSAppendFileAsync
FSBindMount
FSBindMountAsync
FSBindUnmount
FSBindUnmountAsync
FSCancelAllCommands
FSCancelCommand
FSChangeMode
FSChangeModeAsync
FSDumpLastErrorLog
FSFlushMultiQuota
FSFlushMultiQuotaAsync
FSGetEntryNum
FSGetEntryNumAsync
FSGetFileBlockAddress
FSGetFileBlockAddressAsync
FSGetFileSystemInfo
FSGetFileSystemInfoAsync
FSGetVolumeInfo
FSGetVolumeInfoAsync
FSMakeLink
FSMakeLinkAsync
FSMakeQuota
FSMakeQuotaAsync
FSOpenFileByStat
FSOpenFileByStatAsync
FSRegisterFlushQuota
FSRegisterFlushQuotaAsync
FSRemoveQuota
FSRemoveQuotaAsync
FSRollbackQuota
FSRollbackQuotaAsync
FSTimeToCalendarTime
*/

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_fs Filesystem
 * \ingroup coreinit
 *
 * The typical flow of a FS command looks like:
 *
 * {Game thread} FSOpenFileExAsync
 *    -> fsClientSubmitCommand (fsCmdBlockFinishCmd)
 *       -> fsCmdQueueProcessMsg
 *          -> fsClientHandleDequeuedCommand
 *             -> fsaShimSubmitRequestAsync
 *                -> IOS_IoctlAsync (fsClientHandleFsaAsyncCallback)
 *
 * {IPC interrupt} fsClientHandleFsaAsyncCallback
 *    -> SendMessage AppIOQueue FsCmdHandler
 *
 * {AppIo thread} receive FsCmdHandler
 *    -> fsCmdBlockHandleResult
 *       -> fsCmdBlockReplyResult
 *          -> blockBody->finishCmdFn (fsCmdBlockFinishCmd)
 *             -> fsCmdBlockSetResult
 *                -> SendMessage asyncData.ioMsgQueue FsCmdAsync
 *
 * {AppIo thread} receive FsCmdAsync
 *    -> FSGetAsyncResult(msg)->userParams.callback
 *
 * @{
 */

#pragma pack(push, 1)

struct FSClient;
struct FSClientBody;
struct FSCmdBlock;
struct FSCmdBlockBody;

struct FSAsyncData;
struct FSAsyncResult;
struct FSMessage;

struct FSMountSource;

using FSDirEntry = ios::fs::FSADirEntry;
using FSDirHandle = ios::fs::FSADirHandle;
using FSEntryNum = ios::fs::FSAEntryNum;
using FSFileHandle = ios::fs::FSAFileHandle;
using FSFilePosition = ios::fs::FSAFilePosition;
using FSReadFlag = ios::fs::FSAReadFlag;
using FSStat = ios::fs::FSAStat;
using FSWriteFlag = ios::fs::FSAWriteFlag;

using FSAsyncCallbackFn = virt_func_ptr<void(virt_ptr<FSClient>,
                                             virt_ptr<FSCmdBlock>,
                                             FSStatus,
                                             virt_ptr<void>)>;

static constexpr uint32_t
FSMaxBytesPerRequest = 0x100000u;

static constexpr uint32_t
FSMaxPathLength = ios::fs::FSAPathLength;

static constexpr uint32_t
FSMaxMountPathLength = 0x80u;

static constexpr uint8_t
FSMinPriority = 0u;

static constexpr uint8_t
FSDefaultPriority = 16u;

static constexpr uint8_t
FSMaxPriority = 32u;

struct FSMessage
{
   //! Message data
   be2_virt_ptr<void> data;

   UNKNOWN(8);

   //! Type of message
   be2_val<OSFunctionType> type;
};
CHECK_OFFSET(FSMessage, 0x00, data);
CHECK_OFFSET(FSMessage, 0x0C, type);
CHECK_SIZE(FSMessage, 0x10);


/**
 * Async data passed to an FS*Async function.
 */
struct FSAsyncData
{
   //! Callback to call when the command is complete.
   be2_val<FSAsyncCallbackFn> userCallback;

   //! Callback context
   be2_virt_ptr<void> userContext;

   //! Queue to put a message on when command is complete.
   be2_virt_ptr<OSMessageQueue> ioMsgQueue;
};
CHECK_OFFSET(FSAsyncData, 0x00, userCallback);
CHECK_OFFSET(FSAsyncData, 0x04, userContext);
CHECK_OFFSET(FSAsyncData, 0x08, ioMsgQueue);
CHECK_SIZE(FSAsyncData, 0xC);


/**
 * Stores the result of an async FS command.
 */
struct FSAsyncResult
{
   //! User supplied async data.
   be2_struct<FSAsyncData> asyncData;

   //! Message to put into asyncdata.ioMsgQueue.
   be2_struct<FSMessage> ioMsg;

   //! FSClient which owns this result.
   be2_virt_ptr<FSClient> client;

   //! FSCmdBlock which owns this result.
   be2_virt_ptr<FSCmdBlock> block;

   //! The result of the command.
   be2_val<FSStatus> status;
};
CHECK_OFFSET(FSAsyncResult, 0x00, asyncData);
CHECK_OFFSET(FSAsyncResult, 0x0c, ioMsg);
CHECK_OFFSET(FSAsyncResult, 0x1c, client);
CHECK_OFFSET(FSAsyncResult, 0x20, block);
CHECK_OFFSET(FSAsyncResult, 0x24, status);
CHECK_SIZE(FSAsyncResult, 0x28);


/**
 * Information about a mount.
 */
struct FSMountSource
{
   //! Mount type
   be2_val<FSMountSourceType> sourceType;

   //! Mount path
   be2_array<char, FSMaxPathLength - 1> path;
};
CHECK_OFFSET(FSMountSource, 0x0, sourceType);
CHECK_OFFSET(FSMountSource, 0x4, path);
CHECK_SIZE(FSMountSource, 0x283);

#pragma pack(pop)

void
FSInit();

void
FSShutdown();

virt_ptr<FSAsyncResult>
FSGetAsyncResult(virt_ptr<OSMessage> message);

uint32_t
FSGetClientNum();

namespace internal
{

bool
fsInitialised();

bool
fsClientRegistered(virt_ptr<FSClient> client);

bool
fsClientRegistered(virt_ptr<FSClientBody> clientBody);

bool
fsRegisterClient(virt_ptr<FSClientBody> clientBody);

bool
fsDeregisterClient(virt_ptr<FSClientBody> clientBody);

FSStatus
fsAsyncResultInit(virt_ptr<FSClientBody> clientBody,
                  virt_ptr<FSAsyncResult> asyncResult,
                  virt_ptr<const FSAsyncData> asyncData);

} // namespace internal

/** @} */

} // namespace cafe::coreinit

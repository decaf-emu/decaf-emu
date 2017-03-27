#pragma once
#include "coreinit_enum.h"
#include "coreinit_messagequeue.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/be_ptr.h>
#include <common/structsize.h>

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

namespace coreinit
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

struct FSDirEntry;
struct FSStat;
struct FSMountSource;

using FSDirHandle = uint32_t;
using FSEntryNum = uint32_t;
using FSFileHandle = uint32_t;
using FSFilePosition = uint32_t;
using FSAsyncCallbackFn = wfunc_ptr<void, FSClient *, FSCmdBlock *, FSStatus, void *>;

static constexpr uint32_t
FSMaxBytesPerRequest = 0x100000;

static constexpr uint32_t
FSMaxPathLength = 0x27F;

static constexpr uint32_t
FSMaxMountPathLength = 0x80;

static constexpr uint32_t
FSMinPriority = 0;

static constexpr uint32_t
FSDefaultPriority = 16;

static constexpr uint32_t
FSMaxPriority = 32;

struct FSMessage
{
   //! Message data
   be_ptr<void> data;

   UNKNOWN(8);

   //! Type of message
   be_val<OSFunctionType> type;
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
   FSAsyncCallbackFn::be userCallback;

   //! Callback context
   be_ptr<void> userContext;

   //! Queue to put a message on when command is complete.
   be_ptr<OSMessageQueue> ioMsgQueue;
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
   FSAsyncData asyncData;

   //! Message to put into asyncdata.ioMsgQueue.
   FSMessage ioMsg;

   //! FSClient which owns this result.
   be_ptr<FSClient> client;

   //! FSCmdBlock which owns this result.
   be_ptr<FSCmdBlock> block;

   //! The result of the command.
   be_val<FSStatus> status;
};
CHECK_OFFSET(FSAsyncResult, 0x00, asyncData);
CHECK_OFFSET(FSAsyncResult, 0x0c, ioMsg);
CHECK_OFFSET(FSAsyncResult, 0x1c, client);
CHECK_OFFSET(FSAsyncResult, 0x20, block);
CHECK_OFFSET(FSAsyncResult, 0x24, status);
CHECK_SIZE(FSAsyncResult, 0x28);


/**
 * File System information.
 */
struct FSFileSystemInfo
{
   UNKNOWN(0x1C);
};
CHECK_SIZE(FSFileSystemInfo, 0x1C);


/**
 * Information about a mount.
 */
struct FSMountSource
{
   be_val<FSMountSourceType> sourceType;
   char path[FSMaxPathLength];
};
CHECK_OFFSET(FSMountSource, 0x0, sourceType);
CHECK_OFFSET(FSMountSource, 0x4, path);
CHECK_SIZE(FSMountSource, 0x283);


/**
 * Structure used by FSGetStat to return information about a file or directory.
 */
struct FSStat
{
   be_val<FSStatFlags> flags;
   be_val<uint32_t> permission;
   be_val<uint32_t> owner;
   be_val<uint32_t> group;
   be_val<uint32_t> size;
   UNKNOWN(0xC);
   be_val<uint32_t> entryId;
   be_val<OSTime> created;
   be_val<OSTime> modified;
   UNKNOWN(0x30);
};
CHECK_OFFSET(FSStat, 0x00, flags);
CHECK_OFFSET(FSStat, 0x04, permission);
CHECK_OFFSET(FSStat, 0x08, owner);
CHECK_OFFSET(FSStat, 0x0C, group);
CHECK_OFFSET(FSStat, 0x10, size);
CHECK_OFFSET(FSStat, 0x20, entryId);
CHECK_OFFSET(FSStat, 0x24, created);
CHECK_OFFSET(FSStat, 0x2C, modified);
CHECK_SIZE(FSStat, 0x64);


/**
 * Structure used by FSReadDir to iterate the contents of a directory.
 */
struct FSDirEntry
{
   //! File stat.
   FSStat stat;

   //! File name.
   char name[256];
};
CHECK_OFFSET(FSDirEntry, 0x00, stat);
CHECK_OFFSET(FSDirEntry, 0x64, name);
CHECK_SIZE(FSDirEntry, 0x164);

#pragma pack(pop)

void
FSInit();

void
FSShutdown();

FSAsyncResult *
FSGetAsyncResult(OSMessage *message);

uint32_t
FSGetClientNum();

namespace internal
{

bool
fsInitialised();

bool
fsClientRegistered(FSClient *client);

bool
fsClientRegistered(FSClientBody *clientBody);

bool
fsRegisterClient(FSClientBody *clientBody);

bool
fsDeregisterClient(FSClientBody *clientBody);

FSStatus
fsAsyncResultInit(FSClientBody *clientBody,
                  FSAsyncResult *asyncResult,
                  const FSAsyncData *asyncData);

FSStatus
fsDecodeFsaStatusToFsStatus(FSAStatus error);

} // namespace internal

/** @} */

} // namespace coreinit

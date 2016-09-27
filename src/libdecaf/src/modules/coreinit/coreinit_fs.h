#pragma once
#include "common/types.h"
#include "coreinit_enum.h"
#include "coreinit_messagequeue.h"
#include "common/be_val.h"
#include "common/structsize.h"
#include "ppcutils/wfunc_ptr.h"
#include <functional>

namespace coreinit
{

/**
 * \defgroup coreinit_fs Filesystem
 * \ingroup coreinit
 */

using FSDirectoryHandle = uint32_t;
using FSFileHandle = uint32_t;
using FSPriority = uint32_t;

/*
Unimplemented filesystem functions:
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
FSFlushFile
FSFlushFileAsync
FSFlushMultiQuota
FSFlushMultiQuotaAsync
FSFlushQuota
FSFlushQuotaAsync
FSGetCmdPriority
FSGetCurrentCmdBlock
FSGetCwdAsync
FSGetDirSize
FSGetDirSizeAsync
FSGetEmulatedError
FSGetEntryNum
FSGetEntryNumAsync
FSGetFSMessage
FSGetFileBlockAddress
FSGetFileBlockAddressAsync
FSGetFileSystemInfo
FSGetFileSystemInfoAsync
FSGetFreeSpaceSize
FSGetFreeSpaceSizeAsync
FSGetLastError
FSGetMountSource
FSGetMountSourceAsync
FSGetMountSourceNext
FSGetMountSourceNextAsync
FSGetStateChangeInfo
FSGetUserData
FSGetVolumeInfo
FSGetVolumeInfoAsync
FSMakeLink
FSMakeLinkAsync
FSMakeQuota
FSMakeQuotaAsync
FSMount
FSMountAsync
FSOpenFileByStat
FSOpenFileByStatAsync
FSOpenFileEx
FSOpenFileExAsync
FSRegisterFlushQuota
FSRegisterFlushQuotaAsync
FSRemove
FSRemoveAsync
FSRemoveQuota
FSRemoveQuotaAsync
FSRollbackQuota
FSRollbackQuotaAsync
FSSetEmulatedError
FSSetUserData
FSTimeToCalendarTime
FSUnmount
FSUnmountAsync
*/

#pragma pack(push, 1)

class FSClient;
struct FSCmdBlock;

struct FSAsyncData
{
   be_val<uint32_t> callback;
   be_val<uint32_t> param;
   be_ptr<OSMessageQueue> queue;
};
CHECK_OFFSET(FSAsyncData, 0x00, callback);
CHECK_OFFSET(FSAsyncData, 0x04, param);
CHECK_OFFSET(FSAsyncData, 0x08, queue);
CHECK_SIZE(FSAsyncData, 0xC);

struct FSAsyncResult
{
   FSAsyncData userParams;
   OSMessage ioMsg;
   be_ptr<FSClient> client;
   be_ptr<FSCmdBlock> block;
   FSStatus status;
};
CHECK_OFFSET(FSAsyncResult, 0x00, userParams);
CHECK_OFFSET(FSAsyncResult, 0x0c, ioMsg);
CHECK_OFFSET(FSAsyncResult, 0x1c, client);
CHECK_OFFSET(FSAsyncResult, 0x20, block);
CHECK_OFFSET(FSAsyncResult, 0x24, status);
CHECK_SIZE(FSAsyncResult, 0x28);

struct FSCmdBlock
{
   static const unsigned MaxPathLength = 0x280;
   static const unsigned MaxModeLength = 0x10;

   // HACK: We store our own stuff into PPC memory...  This is
   //  especially bad as std::function is not really meant to be
   //  randomly memset...
   uint32_t priority;
   FSAsyncResult result;
   std::function<FSStatus()> func;
   OSMessageQueue syncQueue;
   OSMessage syncQueueMsgs[1];
   char path[MaxPathLength];
   char mode[MaxModeLength];
   char path2[MaxPathLength];
   be_ptr<void> userData;
   UNKNOWN(0x4F4 - sizeof(std::function<FSStatus()>));
};
CHECK_SIZE(FSCmdBlock, 0xa80);

struct FSStat
{
   enum Flags
   {
      Directory = 0x80000000,
   };

   be_val<uint32_t> flags;
   UNKNOWN(0xc);
   be_val<uint32_t> size;
   UNKNOWN(0x50);
};
CHECK_OFFSET(FSStat, 0x00, flags);
CHECK_OFFSET(FSStat, 0x10, size);
CHECK_SIZE(FSStat, 0x64);

struct FSStateChangeInfo
{
   UNKNOWN(0xC);
};
CHECK_SIZE(FSStateChangeInfo, 0xC);

struct FSDirectoryEntry
{
   FSStat info;
   char name[256];
};
CHECK_OFFSET(FSDirectoryEntry, 0x64, name);
CHECK_SIZE(FSDirectoryEntry, 0x164);

using FSAsyncCallback = wfunc_ptr<void, FSClient *, FSCmdBlock *, FSStatus, uint32_t>;

#pragma pack(pop)

void
FSInit();

void
FSShutdown();

void
FSInitCmdBlock(FSCmdBlock *block);

FSStatus
FSSetCmdPriority(FSCmdBlock *block,
                 FSPriority priority);

FSPriority
FSGetCmdPriority(FSCmdBlock *block);

/** @} */

namespace internal
{

void
startFsThread();

void
shutdownFsThread();

void
handleFsDoneInterrupt();

FSAsyncData *
prepareSyncOp(FSClient *client,
              FSCmdBlock *block);

FSStatus
resolveSyncOp(FSClient *client,
              FSCmdBlock *block);

void
queueFsWork(FSClient *client,
            FSCmdBlock *block,
            FSAsyncData *asyncData,
            std::function<FSStatus()> func);

bool
cancelFsWork(FSCmdBlock *cmd);

void
cancelAllFsWork();

} // namespace internal

} // namespace coreinit

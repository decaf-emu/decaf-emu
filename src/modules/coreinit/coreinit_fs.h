#pragma once
#include "types.h"
#include "coreinit_enum.h"
#include "utils/be_val.h"
#include "utils/structsize.h"
#include "utils/wfunc_ptr.h"

namespace coreinit
{

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
FSGetAsyncResult
FSGetCmdPriority
FSGetCurrentCmdBlock
FSGetCwdAsync
FSGetDirSize
FSGetDirSizeAsync
FSGetEmulatedError
FSGetEntryNum
FSGetEntryNumAsync
FSGetErrorCodeForViewer
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
FSRename
FSRenameAsync
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

struct FSCmdBlock
{
   UNKNOWN(0xa80);
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

struct FSAsyncData
{
   be_val<uint32_t> callback;
   be_val<uint32_t> param;
   be_val<uint32_t> unk1;
};
CHECK_OFFSET(FSAsyncData, 0x00, callback);
CHECK_OFFSET(FSAsyncData, 0x04, param);
CHECK_SIZE(FSAsyncData, 0xC);

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

} // namespace coreinit

#pragma once
#include "coreinit_enum.h"
#include "coreinit_fs.h"
#include "coreinit_fsa_shim.h"
#include "coreinit_messagequeue.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/be_ptr.h>
#include <common/structsize.h>

namespace coreinit
{

/**
 * \ingroup coreinit_fs
 * @{
 */

#pragma pack(push, 1)

struct FSClientBody;
struct FSCmdBlock;
struct FSCmdBlockBody;
struct FSCmdQueue;

using FSFinishCmdFn = wfunc_ptr<void, FSCmdBlockBody *, FSStatus>;

struct FSCmdBlock
{
   char data[0xA80];
};
CHECK_SIZE(FSCmdBlock, 0xA80);

struct FSCmdBlockBodyLink
{
   be_ptr<FSCmdBlockBody> next;
   be_ptr<FSCmdBlockBody> prev;
};
CHECK_OFFSET(FSCmdBlockBodyLink, 0x00, next);
CHECK_OFFSET(FSCmdBlockBodyLink, 0x04, prev);
CHECK_SIZE(FSCmdBlockBodyLink, 0x8);

struct FSCmdBlockCmdDataGetCwd
{
   be_ptr<char> returnedPath;
   be_val<uint32_t> bytes;
};
CHECK_OFFSET(FSCmdBlockCmdDataGetCwd, 0x0, returnedPath);
CHECK_OFFSET(FSCmdBlockCmdDataGetCwd, 0x4, bytes);
CHECK_SIZE(FSCmdBlockCmdDataGetCwd, 0x8);

struct FSCmdBlockCmdDataGetFileBlockAddress
{
   be_ptr<be_val<uint32_t>> address;
};
CHECK_OFFSET(FSCmdBlockCmdDataGetFileBlockAddress, 0x0, address);
CHECK_SIZE(FSCmdBlockCmdDataGetFileBlockAddress, 0x4);

union FSCmdBlockCmdDataGetInfoByQuery
{
   be_ptr<void> out;
   be_ptr<be_val<uint64_t>> dirSize;
   be_ptr<be_val<FSEntryNum>> entryNum;
   be_ptr<FSFileSystemInfo> fileSystemInfo;
   be_ptr<be_val<uint64_t>> freeSpaceSize;
   be_ptr<FSStat> stat;
};
CHECK_OFFSET(FSCmdBlockCmdDataGetInfoByQuery, 0x0, out);
CHECK_OFFSET(FSCmdBlockCmdDataGetInfoByQuery, 0x0, dirSize);
CHECK_OFFSET(FSCmdBlockCmdDataGetInfoByQuery, 0x0, entryNum);
CHECK_OFFSET(FSCmdBlockCmdDataGetInfoByQuery, 0x0, fileSystemInfo);
CHECK_OFFSET(FSCmdBlockCmdDataGetInfoByQuery, 0x0, freeSpaceSize);
CHECK_OFFSET(FSCmdBlockCmdDataGetInfoByQuery, 0x0, stat);
CHECK_SIZE(FSCmdBlockCmdDataGetInfoByQuery, 0x4);

struct FSCmdBlockCmdDataGetMountSourceNext
{
   be_ptr<FSMountSource> source;
   be_val<FSDirHandle> dirHandle;
   be_val<FSStatus> readError;
};
CHECK_OFFSET(FSCmdBlockCmdDataGetMountSourceNext, 0x0, source);
CHECK_OFFSET(FSCmdBlockCmdDataGetMountSourceNext, 0x4, dirHandle);
CHECK_OFFSET(FSCmdBlockCmdDataGetMountSourceNext, 0x8, readError);
CHECK_SIZE(FSCmdBlockCmdDataGetMountSourceNext, 0xC);

struct FSCmdBlockCmdDataGetPosFile
{
   be_ptr<be_val<FSFilePosition>> pos;
};
CHECK_OFFSET(FSCmdBlockCmdDataGetPosFile, 0x0, pos);
CHECK_SIZE(FSCmdBlockCmdDataGetPosFile, 0x4);

struct FSCmdBlockCmdDataGetVolumeInfo
{
   be_ptr<FSAVolumeInfo> info;
};
CHECK_OFFSET(FSCmdBlockCmdDataGetVolumeInfo, 0x0, info);
CHECK_SIZE(FSCmdBlockCmdDataGetVolumeInfo, 0x4);

struct FSCmdBlockCmdDataOpenDir
{
   be_ptr<be_val<FSDirHandle>> handle;
};
CHECK_OFFSET(FSCmdBlockCmdDataOpenDir, 0x0, handle);
CHECK_SIZE(FSCmdBlockCmdDataOpenDir, 0x4);

struct FSCmdBlockCmdDataOpenFile
{
   be_ptr<be_val<FSFileHandle>> handle;
};
CHECK_OFFSET(FSCmdBlockCmdDataOpenFile, 0x0, handle);
CHECK_SIZE(FSCmdBlockCmdDataOpenFile, 0x4);

struct FSCmdBlockCmdDataReadDir
{
   be_ptr<FSDirEntry> entry;
};
CHECK_OFFSET(FSCmdBlockCmdDataReadDir, 0x0, entry);
CHECK_SIZE(FSCmdBlockCmdDataReadDir, 0x4);

struct FSCmdBlockCmdDataReadFile
{
   UNKNOWN(4);

   //! Total number of bytes remaining to read.
   be_val<uint32_t> bytesRemaining;

   //! Total bytes read so far.
   be_val<uint32_t> bytesRead;

   //! The size of each read chunk (size parameter on FSReadFile).
   be_val<uint32_t> chunkSize;

   //! The amount of bytes to read per IPC request.
   be_val<uint32_t> readSize;
};
CHECK_OFFSET(FSCmdBlockCmdDataReadFile, 0x4, bytesRemaining);
CHECK_OFFSET(FSCmdBlockCmdDataReadFile, 0x8, bytesRead);
CHECK_OFFSET(FSCmdBlockCmdDataReadFile, 0xC, chunkSize);
CHECK_OFFSET(FSCmdBlockCmdDataReadFile, 0x10, readSize);
CHECK_SIZE(FSCmdBlockCmdDataReadFile, 0x14);

struct FSCmdBlockCmdDataStatFile
{
   be_ptr<FSStat> stat;
};
CHECK_OFFSET(FSCmdBlockCmdDataStatFile, 0x0, stat);
CHECK_SIZE(FSCmdBlockCmdDataStatFile, 0x4);

struct FSCmdBlockCmdDataWriteFile
{
   UNKNOWN(4);

   //! Total number of bytes remaining to write.
   be_val<uint32_t> bytesRemaining;

   //! Total bytes written so far.
   be_val<uint32_t> bytesWritten;

   //! The size of each write chunk (size parameter on FSWriteFile).
   be_val<uint32_t> chunkSize;

   //! The amount of bytes to write per IPC request.
   be_val<uint32_t> writeSize;
};
CHECK_OFFSET(FSCmdBlockCmdDataWriteFile, 0x4, bytesRemaining);
CHECK_OFFSET(FSCmdBlockCmdDataWriteFile, 0x8, bytesWritten);
CHECK_OFFSET(FSCmdBlockCmdDataWriteFile, 0xC, chunkSize);
CHECK_OFFSET(FSCmdBlockCmdDataWriteFile, 0x10, writeSize);
CHECK_SIZE(FSCmdBlockCmdDataWriteFile, 0x14);

/**
 * Stores command specific data for an FSCmdBlockBody.
 */
union FSCmdBlockCmdData
{
   FSCmdBlockCmdDataGetCwd getCwd;
   FSCmdBlockCmdDataGetFileBlockAddress getFileBlockAddress;
   FSCmdBlockCmdDataGetInfoByQuery getInfoByQuery;
   FSCmdBlockCmdDataGetMountSourceNext getMountSourceNext;
   FSCmdBlockCmdDataGetPosFile getPosFile;
   FSCmdBlockCmdDataGetVolumeInfo getVolumeInfo;
   FSCmdBlockCmdDataOpenDir openDir;
   FSCmdBlockCmdDataOpenFile openFile;
   FSCmdBlockCmdDataReadDir readDir;
   FSCmdBlockCmdDataReadFile readFile;
   FSCmdBlockCmdDataStatFile statFile;
   FSCmdBlockCmdDataWriteFile writeFile;
   UNKNOWN(0x14);
};
CHECK_SIZE(FSCmdBlockCmdData, 0x14);

struct FSCmdBlockBody
{
   //! FSA shim buffer used for FSA IPC communication.
   FSAShimBuffer fsaShimBuffer;

   //! Pointer to client which owns this command.
   be_ptr<FSClientBody> clientBody;

   //! State of command.
   be_val<FSCmdBlockStatus> status;

   //! Cancel state of command.
   be_val<FSCmdCancelFlags> cancelFlags;

   //! Command specific data.
   FSCmdBlockCmdData cmdData;

   //! Link used for FSCmdQueue.
   FSCmdBlockBodyLink link;

   //! FSAStatus for the current command.
   be_val<FSAStatus> fsaStatus;

   //! IOSError for the current command.
   be_val<IOSError> iosError;

   //! Mask used to not return certain errors.
   be_val<FSErrorFlag> errorMask;

   //! FSAsyncResult object used for this command.
   FSAsyncResult asyncResult;

   //! User data accessed with FS{Get,Set}UserData.
   be_ptr<void> userData;

   //! Queue used for synchronous FS commands to wait for finish.
   OSMessageQueue syncQueue;

   //! Message used for syncQueue.
   OSMessage syncQueueMsgs[1];

   //! Callback to call when command is finished.
   FSFinishCmdFn::be finishCmdFn;

   //! Priority of command, from 0 highest to 32 lowest, 16 is default.
   be_val<uint8_t> priority;

   be_val<uint8_t> unk0x9E9;
   be_val<uint8_t> unk0x9EA;
   UNKNOWN(0x9);
   be_val<uint32_t> unk0x9F4;

   //! Pointer to unaligned FSCmdBlock.
   be_ptr<FSCmdBlock> cmdBlock;
};
CHECK_OFFSET(FSCmdBlockBody, 0x0, fsaShimBuffer);
CHECK_OFFSET(FSCmdBlockBody, 0x938, clientBody);
CHECK_OFFSET(FSCmdBlockBody, 0x93C, status);
CHECK_OFFSET(FSCmdBlockBody, 0x940, cancelFlags);
CHECK_OFFSET(FSCmdBlockBody, 0x944, cmdData);
CHECK_OFFSET(FSCmdBlockBody, 0x960, fsaStatus);
CHECK_OFFSET(FSCmdBlockBody, 0x964, iosError);
CHECK_OFFSET(FSCmdBlockBody, 0x968, errorMask);
CHECK_OFFSET(FSCmdBlockBody, 0x96C, asyncResult);
CHECK_OFFSET(FSCmdBlockBody, 0x994, userData);
CHECK_OFFSET(FSCmdBlockBody, 0x998, syncQueue);
CHECK_OFFSET(FSCmdBlockBody, 0x9D4, syncQueueMsgs);
CHECK_OFFSET(FSCmdBlockBody, 0x9E4, finishCmdFn);
CHECK_OFFSET(FSCmdBlockBody, 0x9E8, priority);
CHECK_OFFSET(FSCmdBlockBody, 0x9E9, unk0x9E9);
CHECK_OFFSET(FSCmdBlockBody, 0x9EA, unk0x9EA);
CHECK_OFFSET(FSCmdBlockBody, 0x9F4, unk0x9F4);
CHECK_OFFSET(FSCmdBlockBody, 0x9F8, cmdBlock);

#pragma pack(pop)

void
FSInitCmdBlock(FSCmdBlock *block);

FSStatus
FSGetCmdPriority(FSCmdBlock *block);

FSStatus
FSSetCmdPriority(FSCmdBlock *block,
                 uint32_t priority);

FSMessage *
FSGetFSMessage(FSCmdBlock *block);

void *
FSGetUserData(FSCmdBlock *block);

void
FSSetUserData(FSCmdBlock *block,
              void *userData);

namespace internal
{

FSCmdBlockBody *
fsCmdBlockGetBody(FSCmdBlock *cmdBlock);

FSStatus
fsCmdBlockPrepareAsync(FSClientBody *clientBody,
                       FSCmdBlockBody *blockBody,
                       FSErrorFlag errorMask,
                       const FSAsyncData *asyncData);

void
fsCmdBlockPrepareSync(FSClient *client,
                      FSCmdBlock *block,
                      FSAsyncData *asyncData);

void
fsCmdBlockRequeue(FSCmdQueue *queue,
                  FSCmdBlockBody *blockBody,
                  BOOL insertAtFront,
                  FSFinishCmdFn finishCmdFn);

void
fsCmdBlockSetResult(FSCmdBlockBody *blockBody,
                    FSStatus status);

void
fsCmdBlockReplyResult(FSCmdBlockBody *blockBody,
                      FSStatus status);

void
fsCmdBlockHandleResult(FSCmdBlockBody *blockBody);

extern FSFinishCmdFn
fsCmdBlockFinishCmdFn;

extern FSFinishCmdFn
fsCmdBlockFinishMountCmdFn;

extern FSFinishCmdFn
fsCmdBlockFinishReadCmdFn;

extern FSFinishCmdFn
fsCmdBlockFinishWriteCmdFn;

extern FSFinishCmdFn
fsCmdBlockFinishGetMountSourceNextOpenCmdFn;

extern FSFinishCmdFn
fsCmdBlockFinishGetMountSourceNextReadCmdFn;

extern FSFinishCmdFn
fsCmdBlockFinishGetMountSourceNextCloseCmdFn;

} // namespace internal

/** @} */

} // namespace coreinit

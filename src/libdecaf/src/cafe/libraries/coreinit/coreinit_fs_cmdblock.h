#pragma once
#include "coreinit_enum.h"
#include "coreinit_fs.h"
#include "coreinit_fsa.h"
#include "coreinit_fsa_shim.h"
#include "coreinit_messagequeue.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
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

using FSFinishCmdFn = virt_func_ptr<void(virt_ptr<FSCmdBlockBody>,
                                         FSStatus)>;

struct FSCmdBlock
{
   be2_array<char, 0xA80> data;
};
CHECK_SIZE(FSCmdBlock, 0xA80);

struct FSCmdBlockBodyLink
{
   be2_virt_ptr<FSCmdBlockBody> next;
   be2_virt_ptr<FSCmdBlockBody> prev;
};
CHECK_OFFSET(FSCmdBlockBodyLink, 0x00, next);
CHECK_OFFSET(FSCmdBlockBodyLink, 0x04, prev);
CHECK_SIZE(FSCmdBlockBodyLink, 0x8);

struct FSCmdBlockCmdDataGetCwd
{
   be2_virt_ptr<char> returnedPath;
   be2_val<uint32_t> bytes;
};
CHECK_OFFSET(FSCmdBlockCmdDataGetCwd, 0x0, returnedPath);
CHECK_OFFSET(FSCmdBlockCmdDataGetCwd, 0x4, bytes);
CHECK_SIZE(FSCmdBlockCmdDataGetCwd, 0x8);

struct FSCmdBlockCmdDataGetFileBlockAddress
{
   be2_virt_ptr<uint32_t> address;
};
CHECK_OFFSET(FSCmdBlockCmdDataGetFileBlockAddress, 0x0, address);
CHECK_SIZE(FSCmdBlockCmdDataGetFileBlockAddress, 0x4);

struct FSCmdBlockCmdDataGetInfoByQuery
{
   union
   {
      be2_virt_ptr<void> out;
      be2_virt_ptr<uint64_t> dirSize;
      be2_virt_ptr<FSEntryNum> entryNum;
      be2_virt_ptr<FSAFileSystemInfo> fileSystemInfo;
      be2_virt_ptr<uint64_t> freeSpaceSize;
      be2_virt_ptr<FSStat> stat;
   };
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
   be2_virt_ptr<FSMountSource> source;
   be2_val<FSDirHandle> dirHandle;
   be2_val<FSStatus> readError;
};
CHECK_OFFSET(FSCmdBlockCmdDataGetMountSourceNext, 0x0, source);
CHECK_OFFSET(FSCmdBlockCmdDataGetMountSourceNext, 0x4, dirHandle);
CHECK_OFFSET(FSCmdBlockCmdDataGetMountSourceNext, 0x8, readError);
CHECK_SIZE(FSCmdBlockCmdDataGetMountSourceNext, 0xC);

struct FSCmdBlockCmdDataGetPosFile
{
   be2_virt_ptr<FSFilePosition> pos;
};
CHECK_OFFSET(FSCmdBlockCmdDataGetPosFile, 0x0, pos);
CHECK_SIZE(FSCmdBlockCmdDataGetPosFile, 0x4);

struct FSCmdBlockCmdDataGetVolumeInfo
{
   be2_virt_ptr<FSAVolumeInfo> info;
};
CHECK_OFFSET(FSCmdBlockCmdDataGetVolumeInfo, 0x0, info);
CHECK_SIZE(FSCmdBlockCmdDataGetVolumeInfo, 0x4);

struct FSCmdBlockCmdDataOpenDir
{
   be2_virt_ptr<FSDirHandle> handle;
};
CHECK_OFFSET(FSCmdBlockCmdDataOpenDir, 0x0, handle);
CHECK_SIZE(FSCmdBlockCmdDataOpenDir, 0x4);

struct FSCmdBlockCmdDataOpenFile
{
   be2_virt_ptr<FSFileHandle> handle;
};
CHECK_OFFSET(FSCmdBlockCmdDataOpenFile, 0x0, handle);
CHECK_SIZE(FSCmdBlockCmdDataOpenFile, 0x4);

struct FSCmdBlockCmdDataReadDir
{
   be2_virt_ptr<FSDirEntry> entry;
};
CHECK_OFFSET(FSCmdBlockCmdDataReadDir, 0x0, entry);
CHECK_SIZE(FSCmdBlockCmdDataReadDir, 0x4);

struct FSCmdBlockCmdDataReadFile
{
   UNKNOWN(4);

   //! Total number of bytes remaining to read.
   be2_val<uint32_t> bytesRemaining;

   //! Total bytes read so far.
   be2_val<uint32_t> bytesRead;

   //! The size of each read chunk (size parameter on FSReadFile).
   be2_val<uint32_t> chunkSize;

   //! The amount of bytes to read per IPC request.
   be2_val<uint32_t> readSize;
};
CHECK_OFFSET(FSCmdBlockCmdDataReadFile, 0x4, bytesRemaining);
CHECK_OFFSET(FSCmdBlockCmdDataReadFile, 0x8, bytesRead);
CHECK_OFFSET(FSCmdBlockCmdDataReadFile, 0xC, chunkSize);
CHECK_OFFSET(FSCmdBlockCmdDataReadFile, 0x10, readSize);
CHECK_SIZE(FSCmdBlockCmdDataReadFile, 0x14);

struct FSCmdBlockCmdDataStatFile
{
   be2_virt_ptr<FSStat> stat;
};
CHECK_OFFSET(FSCmdBlockCmdDataStatFile, 0x0, stat);
CHECK_SIZE(FSCmdBlockCmdDataStatFile, 0x4);

struct FSCmdBlockCmdDataWriteFile
{
   UNKNOWN(4);

   //! Total number of bytes remaining to write.
   be2_val<uint32_t> bytesRemaining;

   //! Total bytes written so far.
   be2_val<uint32_t> bytesWritten;

   //! The size of each write chunk (size parameter on FSWriteFile).
   be2_val<uint32_t> chunkSize;

   //! The amount of bytes to write per IPC request.
   be2_val<uint32_t> writeSize;
};
CHECK_OFFSET(FSCmdBlockCmdDataWriteFile, 0x4, bytesRemaining);
CHECK_OFFSET(FSCmdBlockCmdDataWriteFile, 0x8, bytesWritten);
CHECK_OFFSET(FSCmdBlockCmdDataWriteFile, 0xC, chunkSize);
CHECK_OFFSET(FSCmdBlockCmdDataWriteFile, 0x10, writeSize);
CHECK_SIZE(FSCmdBlockCmdDataWriteFile, 0x14);

/**
 * Stores command specific data for an FSCmdBlockBody.
 */
struct FSCmdBlockCmdData
{
   union
   {
      be2_struct<FSCmdBlockCmdDataGetCwd> getCwd;
      be2_struct<FSCmdBlockCmdDataGetFileBlockAddress> getFileBlockAddress;
      be2_struct<FSCmdBlockCmdDataGetInfoByQuery> getInfoByQuery;
      be2_struct<FSCmdBlockCmdDataGetMountSourceNext> getMountSourceNext;
      be2_struct<FSCmdBlockCmdDataGetPosFile> getPosFile;
      be2_struct<FSCmdBlockCmdDataGetVolumeInfo> getVolumeInfo;
      be2_struct<FSCmdBlockCmdDataOpenDir> openDir;
      be2_struct<FSCmdBlockCmdDataOpenFile> openFile;
      be2_struct<FSCmdBlockCmdDataReadDir> readDir;
      be2_struct<FSCmdBlockCmdDataReadFile> readFile;
      be2_struct<FSCmdBlockCmdDataStatFile> statFile;
      be2_struct<FSCmdBlockCmdDataWriteFile> writeFile;
      UNKNOWN(0x14);
   };
};
CHECK_SIZE(FSCmdBlockCmdData, 0x14);

struct FSCmdBlockBody
{
   //! FSA shim buffer used for FSA IPC communication.
   be2_struct<FSAShimBuffer> fsaShimBuffer;

   //! Pointer to client which owns this command.
   be2_virt_ptr<FSClientBody> clientBody;

   //! State of command.
   be2_val<FSCmdBlockStatus> status;

   //! Cancel state of command.
   be2_val<FSCmdCancelFlags> cancelFlags;

   //! Command specific data.
   be2_struct<FSCmdBlockCmdData> cmdData;

   //! Link used for FSCmdQueue.
   be2_struct<FSCmdBlockBodyLink> link;

   //! FSAStatus for the current command.
   be2_val<FSAStatus> fsaStatus;

   //! IOSError for the current command.
   be2_val<IOSError> iosError;

   //! Mask used to not return certain errors.
   be2_val<FSErrorFlag> errorMask;

   //! FSAsyncResult object used for this command.
   be2_struct<FSAsyncResult> asyncResult;

   //! User data accessed with FS{Get,Set}UserData.
   be2_virt_ptr<void> userData;

   //! Queue used for synchronous FS commands to wait for finish.
   be2_struct<OSMessageQueue> syncQueue;

   //! Message used for syncQueue.
   be2_array<OSMessage, 1> syncQueueMsgs;

   //! Callback to call when command is finished.
   be2_val<FSFinishCmdFn> finishCmdFn;

   //! Priority of command, from 0 highest to 32 lowest, 16 is default.
   be2_val<uint8_t> priority;

   be2_val<uint8_t> unk0x9E9;
   be2_val<uint8_t> unk0x9EA;
   UNKNOWN(0x9);
   be2_val<uint32_t> unk0x9F4;

   //! Pointer to unaligned FSCmdBlock.
   be2_virt_ptr<FSCmdBlock> cmdBlock;
};
CHECK_OFFSET(FSCmdBlockBody, 0x0, fsaShimBuffer);
CHECK_OFFSET(FSCmdBlockBody, 0x938, clientBody);
CHECK_OFFSET(FSCmdBlockBody, 0x93C, status);
CHECK_OFFSET(FSCmdBlockBody, 0x940, cancelFlags);
CHECK_OFFSET(FSCmdBlockBody, 0x944, cmdData);
CHECK_OFFSET(FSCmdBlockBody, 0x958, link);
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
FSInitCmdBlock(virt_ptr<FSCmdBlock> block);

FSStatus
FSGetCmdPriority(virt_ptr<FSCmdBlock> block);

FSStatus
FSSetCmdPriority(virt_ptr<FSCmdBlock> block,
                 uint32_t priority);

virt_ptr<FSMessage>
FSGetFSMessage(virt_ptr<FSCmdBlock> block);

virt_ptr<void>
FSGetUserData(virt_ptr<FSCmdBlock> block);

void
FSSetUserData(virt_ptr<FSCmdBlock> block,
              virt_ptr<void> userData);

namespace internal
{

extern FSFinishCmdFn FinishCmd;
extern FSFinishCmdFn FinishMountCmd;
extern FSFinishCmdFn FinishReadCmd;
extern FSFinishCmdFn FinishWriteCmd;
extern FSFinishCmdFn FinishGetMountSourceNextOpenCmd;
extern FSFinishCmdFn FinishGetMountSourceNextReadCmd;
extern FSFinishCmdFn FinishGetMountSourceNextCloseCmd;

virt_ptr<FSCmdBlockBody>
fsCmdBlockGetBody(virt_ptr<FSCmdBlock> cmdBlock);

FSStatus
fsCmdBlockPrepareAsync(virt_ptr<FSClientBody> clientBody,
                       virt_ptr<FSCmdBlockBody> blockBody,
                       FSErrorFlag errorMask,
                       virt_ptr<const FSAsyncData> asyncData);

void
fsCmdBlockPrepareSync(virt_ptr<FSClient> client,
                      virt_ptr<FSCmdBlock> block,
                      virt_ptr<FSAsyncData> asyncData);

void
fsCmdBlockRequeue(virt_ptr<FSCmdQueue> queue,
                  virt_ptr<FSCmdBlockBody> blockBody,
                  BOOL insertAtFront,
                  FSFinishCmdFn finishCmdFn);

void
fsCmdBlockSetResult(virt_ptr<FSCmdBlockBody> blockBody,
                    FSStatus status);

void
fsCmdBlockReplyResult(virt_ptr<FSCmdBlockBody> blockBody,
                      FSStatus status);

void
fsCmdBlockHandleResult(virt_ptr<FSCmdBlockBody> blockBody);


void
fsCmdBlockFinishCmd(virt_ptr<FSCmdBlockBody> blockBody,
                    FSStatus status);

void
fsCmdBlockFinishReadCmd(virt_ptr<FSCmdBlockBody> blockBody,
                        FSStatus status);

void
fsCmdBlockFinishWriteCmd(virt_ptr<FSCmdBlockBody> blockBody,
                         FSStatus status);

void
fsCmdBlockFinishMountCmd(virt_ptr<FSCmdBlockBody> blockBody,
                         FSStatus result);

void
fsCmdBlockFinishGetMountSourceNextOpenCmd(virt_ptr<FSCmdBlockBody> blockBody,
                                          FSStatus status);

void
fsCmdBlockFinishGetMountSourceNextReadCmd(virt_ptr<FSCmdBlockBody> blockBody,
                                          FSStatus result);

void
fsCmdBlockFinishGetMountSourceNextCloseCmd(virt_ptr<FSCmdBlockBody> blockBody,
                                           FSStatus result);

} // namespace internal

/** @} */

} // namespace cafe::coreinit

#include "coreinit.h"
#include "coreinit_appio.h"
#include "coreinit_fs.h"
#include "coreinit_fs_client.h"
#include "coreinit_fs_driver.h"
#include "coreinit_fs_cmdblock.h"
#include "coreinit_fsa_shim.h"
#include "cafe/cafe_ppc_interface_invoke.h"

#include <cstring>
#include <common/align.h>
#include <common/log.h>
#include <fmt/format.h>
#include <libcpu/cpu.h>

namespace cafe::coreinit
{

namespace internal
{

FSFinishCmdFn FinishCmd = nullptr;
FSFinishCmdFn FinishMountCmd = nullptr;
FSFinishCmdFn FinishReadCmd = nullptr;
FSFinishCmdFn FinishWriteCmd = nullptr;
FSFinishCmdFn FinishGetMountSourceNextOpenCmd = nullptr;
FSFinishCmdFn FinishGetMountSourceNextReadCmd = nullptr;
FSFinishCmdFn FinishGetMountSourceNextCloseCmd = nullptr;

}

/**
 * Initialise an FSCmdBlock structure.
 */
void
FSInitCmdBlock(virt_ptr<FSCmdBlock> block)
{
   if (!block) {
      return;
   }

   std::memset(block.getRawPointer(), 0, sizeof(FSCmdBlock));

   auto blockBody = internal::fsCmdBlockGetBody(block);
   blockBody->status = FSCmdBlockStatus::Initialised;
   blockBody->priority = FSDefaultPriority;
}


/**
 * Get the command's priority.
 *
 * \return
 * Returns positive value on success, or FSStatus error code otherwise.
 */
FSStatus
FSGetCmdPriority(virt_ptr<FSCmdBlock> block)
{
   auto blockBody = internal::fsCmdBlockGetBody(block);
   if (!blockBody) {
      return FSStatus::FatalError;
   }

   return static_cast<FSStatus>(blockBody->priority.value());
}


/**
 * Set the command's priority.
 *
 * \retval FSStatus::OK
 * Success.
 *
 * \retval FSStatus::FatalError
 * FSCmdBlock in invalid state, or invalid priority.
 */
FSStatus
FSSetCmdPriority(virt_ptr<FSCmdBlock> block,
                 uint32_t priority)
{
   auto blockBody = internal::fsCmdBlockGetBody(block);
   if (!blockBody) {
      return FSStatus::FatalError;
   }

   if (priority < FSMinPriority || priority > FSMaxPriority) {
      return FSStatus::FatalError;
   }

   if (blockBody->status != FSCmdBlockStatus::Initialised &&
       blockBody->status != FSCmdBlockStatus::Cancelled) {
      // Cannot adjust a commands priority once it has been queued.
      return FSStatus::FatalError;
   }

   blockBody->priority = static_cast<uint8_t>(priority);
   return FSStatus::OK;
}


/**
 * Get the FSMessage structure in an FSCmdBlock.
 *
 * \return
 * Returns the FSMessage structure from the FSCmdBlock's FSAsyncResult.
 */
virt_ptr<FSMessage>
FSGetFSMessage(virt_ptr<FSCmdBlock> block)
{
   auto blockBody = internal::fsCmdBlockGetBody(block);
   if (!blockBody) {
      return nullptr;
   }

   return virt_addrof(blockBody->asyncResult.ioMsg);
}


/**
 * Get the value stored in FSCmdBlock by FSSetUserData.
 *
 * \return
 * Returns pointer set by FSSetUserData.
 */
virt_ptr<void>
FSGetUserData(virt_ptr<FSCmdBlock> block)
{
   auto blockBody = internal::fsCmdBlockGetBody(block);
   return blockBody->userData;
}


/**
 * Store a user value in FSCmdBlock which can be retrieved by FSGetUserData.
 */
void
FSSetUserData(virt_ptr<FSCmdBlock> block,
              virt_ptr<void> userData)
{
   auto blockBody = internal::fsCmdBlockGetBody(block);
   blockBody->userData = userData;
}


namespace internal
{

/**
 * Get an aligned FSCmdBlockBody from an FSCmdBlock.
 */
virt_ptr<FSCmdBlockBody>
fsCmdBlockGetBody(virt_ptr<FSCmdBlock> cmdBlock)
{
   if (!cmdBlock) {
      return nullptr;
   }

   auto body = virt_cast<FSCmdBlockBody *>(align_up(virt_cast<virt_addr>(cmdBlock), 0x40));
   body->cmdBlock = cmdBlock;
   return body;
}


/**
 * Prepare a FSCmdBlock for an asynchronous operation.
 *
 * \return
 * Returns a positive value on success, FSStatus error code otherwise.
 */
FSStatus
fsCmdBlockPrepareAsync(virt_ptr<FSClientBody> clientBody,
                       virt_ptr<FSCmdBlockBody> blockBody,
                       FSErrorFlag errorMask,
                       virt_ptr<const FSAsyncData> asyncData)
{
   decaf_check(clientBody);
   decaf_check(blockBody);

   if (blockBody->status != FSCmdBlockStatus::Initialised &&
       blockBody->status != FSCmdBlockStatus::Cancelled) {
      gLog->error("Invalid FSCmdBlockData state {}", blockBody->status.value());
      return FSStatus::FatalError;
   }

   if (asyncData->userCallback && asyncData->ioMsgQueue) {
      gLog->error("userCallback and ioMsgQueue are exclusive.");
      return FSStatus::FatalError;
   }

   blockBody->errorMask = errorMask;
   blockBody->clientBody = clientBody;
   return fsAsyncResultInit(clientBody,
                            virt_addrof(blockBody->asyncResult),
                            asyncData);
}


/**
 * Prepare a FSCmdBlock for a synchronous operation.
 */
void
fsCmdBlockPrepareSync(virt_ptr<FSClient> client,
                      virt_ptr<FSCmdBlock> block,
                      virt_ptr<FSAsyncData> asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto blockBody = internal::fsCmdBlockGetBody(block);
   OSInitMessageQueue(virt_addrof(blockBody->syncQueue),
                      virt_addrof(blockBody->syncQueueMsgs),
                      1);

   asyncData->ioMsgQueue = virt_addrof(blockBody->syncQueue);
}


/**
 * Requeues an FS command.
 */
void
fsCmdBlockRequeue(virt_ptr<FSCmdQueue> queue,
                  virt_ptr<FSCmdBlockBody> blockBody,
                  BOOL insertAtFront,
                  FSFinishCmdFn finishCmdFn)
{
   OSFastMutex_Lock(virt_addrof(queue->mutex));

   if (blockBody->cancelFlags & FSCmdCancelFlags::Cancelling) {
      blockBody->cancelFlags &= ~FSCmdCancelFlags::Cancelling;
      blockBody->status = FSCmdBlockStatus::Cancelled;
      blockBody->clientBody->lastDequeuedCommand = nullptr;
      OSFastMutex_Unlock(virt_addrof(queue->mutex));
      fsCmdBlockReplyResult(blockBody, FSStatus::Cancelled);
      return;
   }

   blockBody->finishCmdFn = finishCmdFn;
   blockBody->status = FSCmdBlockStatus::QueuedCommand;
   fsCmdQueueFinishCmd(queue);

   if (insertAtFront) {
      fsCmdQueuePushFront(queue, blockBody);
   } else {
      fsCmdQueueEnqueue(queue, blockBody, true);
   }

   OSFastMutex_Unlock(virt_addrof(queue->mutex));
   fsCmdQueueProcessCmd(queue);
}


/**
 * Set the result for an FSCmd.
 *
 * A message will be sent to the user's ioMsgQueue if one was provided or to
 * the AppIO queue where the user's callback will be called instead.
 */
void
fsCmdBlockSetResult(virt_ptr<FSCmdBlockBody> blockBody,
                    FSStatus status)
{
   blockBody->asyncResult.block = blockBody->cmdBlock;
   blockBody->asyncResult.status = status;

   if (!OSSendMessage(blockBody->asyncResult.asyncData.ioMsgQueue,
                      virt_cast<OSMessage *>(virt_addrof(blockBody->asyncResult.ioMsg)),
                      OSMessageFlags::None)) {
      decaf_abort("fsCmdBlockReplyResult: Could not send async result message");
   }
}


/**
 * Calls the blockBody->finishCmdFn with the result of the command.
 */
void
fsCmdBlockReplyResult(virt_ptr<FSCmdBlockBody> blockBody,
                      FSStatus status)
{
   if (!blockBody) {
      return;
   }

   // Finish the current command
   auto queue = virt_addrof(blockBody->clientBody->cmdQueue);
   OSFastMutex_Lock(virt_addrof(queue->mutex));
   fsCmdQueueFinishCmd(queue);
   OSFastMutex_Unlock(virt_addrof(queue->mutex));

   if (blockBody->finishCmdFn) {
      cafe::invoke(cpu::this_core::state(),
                   blockBody->finishCmdFn,
                   blockBody,
                   status);
   }

   // Start off next command
   fsCmdQueueProcessCmd(queue);
}


/**
 * Called from the AppIO thread to handle the result of an FS command.
 */
void
fsCmdBlockHandleResult(virt_ptr<FSCmdBlockBody> blockBody)
{
   auto clientBody = blockBody->clientBody;
   auto result = static_cast<FSStatus>(blockBody->fsaStatus.value());

   if (!fsClientRegistered(clientBody)) {
      if (blockBody->finishCmdFn) {
         cafe::invoke(cpu::this_core::state(),
                      blockBody->finishCmdFn,
                      blockBody,
                      FSStatus::Cancelled);
      }

      return;
   }

   clientBody->lastError = blockBody->fsaStatus;

   if (blockBody->fsaStatus == FSAStatus::MediaNotReady) {
      fsmSetState(virt_addrof(clientBody->fsm),
                  FSVolumeState::WrongMedia,
                  clientBody);
      return;
   } else if (blockBody->fsaStatus == FSAStatus::WriteProtected) {
      fsmSetState(virt_addrof(clientBody->fsm),
                  FSVolumeState::MediaError,
                  clientBody);
      return;
   }

   if (blockBody->fsaStatus < 0) {
      auto errorFlags = FSErrorFlag::All;

      switch (blockBody->fsaStatus) {
      case FSAStatus::Busy:
         fsCmdBlockRequeue(virt_addrof(clientBody->cmdQueue), blockBody, TRUE,
                           blockBody->finishCmdFn);
         return;
      case FSAStatus::Cancelled:
         result = FSStatus::Cancelled;
         errorFlags = FSErrorFlag::None;
         break;
      case FSAStatus::EndOfDir:
      case FSAStatus::EndOfFile:
         result = FSStatus::End;
         errorFlags = FSErrorFlag::None;
         break;
      case FSAStatus::MaxMountpoints:
      case FSAStatus::MaxVolumes:
      case FSAStatus::MaxClients:
      case FSAStatus::MaxFiles:
      case FSAStatus::MaxDirs:
         errorFlags = FSErrorFlag::Max;
         result = FSStatus::Max;
         break;
      case FSAStatus::AlreadyOpen:
         errorFlags = FSErrorFlag::AlreadyOpen;
         result = FSStatus::AlreadyOpen;
         break;
      case FSAStatus::NotFound:
         errorFlags = FSErrorFlag::NotFound;
         result = FSStatus::NotFound;
         break;
      case FSAStatus::AlreadyExists:
      case FSAStatus::NotEmpty:
         errorFlags = FSErrorFlag::Exists;
         result = FSStatus::Exists;
         break;
      case FSAStatus::AccessError:
         errorFlags = FSErrorFlag::AccessError;
         result = FSStatus::AccessError;
         break;
      case FSAStatus::PermissionError:
         errorFlags = FSErrorFlag::PermissionError;
         result = FSStatus::PermissionError;
         break;
      case FSAStatus::DataCorrupted:
         // TODO: FSAStatus::DataCorrupted
         decaf_abort("TODO: Reverse me.");
         break;
      case FSAStatus::StorageFull:
         errorFlags = FSErrorFlag::StorageFull;
         result = FSStatus::StorageFull;
         break;
      case FSAStatus::JournalFull:
         errorFlags = FSErrorFlag::JournalFull;
         result = FSStatus::JournalFull;
         break;
      case FSAStatus::UnsupportedCmd:
         errorFlags = FSErrorFlag::UnsupportedCmd;
         result = FSStatus::UnsupportedCmd;
         break;
      case FSAStatus::NotFile:
         errorFlags = FSErrorFlag::NotFile;
         result = FSStatus::NotFile;
         break;
      case FSAStatus::NotDir:
         errorFlags = FSErrorFlag::NotDir;
         result = FSStatus::NotDirectory;
         break;
      case FSAStatus::FileTooBig:
         errorFlags = FSErrorFlag::FileTooBig;
         result = FSStatus::FileTooBig;
         break;
      case FSAStatus::MediaError:
         // TODO: FSAStatus::MediaError
         decaf_abort("TODO: Reverse me.");
         break;
      default:
         errorFlags = FSErrorFlag::All;
      }

      if (errorFlags != FSErrorFlag::None &&
          (blockBody->errorMask & errorFlags) == 0) {
         fsmEnterState(virt_addrof(clientBody->fsm),
                       FSVolumeState::Fatal,
                       clientBody);

         // The game told us not to return if we receive this error, so must mean
         // that we really fucked something up :).
         decaf_abort(fmt::format("Unrecoverable FS error, command = {}, error = {}.",
                                 blockBody->fsaShimBuffer.command,
                                 blockBody->fsaStatus));

         return;
      }
   }

   if (clientBody->lastDequeuedCommand == blockBody) {
      clientBody->lastDequeuedCommand = nullptr;
   }

   fsCmdBlockReplyResult(blockBody, result);
}


/**
 * Copies the IOS command results to FS output.
 *
 * Set as blockBlody->finishCmdFn.
 * Called from fsCmdBlockReplyResult.
 */
void
fsCmdBlockFinishCmd(virt_ptr<FSCmdBlockBody> blockBody,
                    FSStatus result)
{
   auto clientBody = blockBody->clientBody;

   OSFastMutex_Lock(virt_addrof(clientBody->mutex));
   blockBody->cancelFlags &= ~FSCmdCancelFlags::Cancelling;

   if (clientBody->lastDequeuedCommand == blockBody) {
      clientBody->lastDequeuedCommand = nullptr;
   }

   blockBody->status = FSCmdBlockStatus::Cancelled;
   OSFastMutex_Unlock(virt_addrof(clientBody->mutex));

   if (result < 0) {
      fsCmdBlockSetResult(blockBody, result);
      return;
   }

   blockBody->unk0x9EA = uint8_t { 0 };
   blockBody->unk0x9F4 = 0u;

   auto &shim = blockBody->fsaShimBuffer;
   switch (shim.command) {
   case FSACommand::Mount:
   case FSACommand::Unmount:
   case FSACommand::ChangeDir:
   case FSACommand::MakeDir:
   case FSACommand::Remove:
   case FSACommand::Rename:
   case FSACommand::RewindDir:
   case FSACommand::CloseDir:
   case FSACommand::ReadFile:
   case FSACommand::WriteFile:
   case FSACommand::SetPosFile:
   case FSACommand::IsEof:
   case FSACommand::CloseFile:
   case FSACommand::GetError:
   case FSACommand::FlushFile:
   case FSACommand::AppendFile:
   case FSACommand::TruncateFile:
   case FSACommand::MakeQuota:
   case FSACommand::FlushQuota:
   case FSACommand::RollbackQuota:
   case FSACommand::ChangeMode:
   case FSACommand::RegisterFlushQuota:
   case FSACommand::FlushMultiQuota:
   case FSACommand::RemoveQuota:
   case FSACommand::MakeLink:
   {
      fsCmdBlockSetResult(blockBody, result);
      break;
   }
   case FSACommand::GetVolumeInfo:
   {
      auto info = blockBody->cmdData.getVolumeInfo.info;
      *info = shim.response.getVolumeInfo.volumeInfo;
      info->unk0x0C = 0u;
      info->unk0x10 = 0u;
      info->unk0x14 = -1;
      info->unk0x18 = -1;
      fsCmdBlockSetResult(blockBody, result);
      break;
   }
   case FSACommand::OpenDir:
   {
      auto handle = blockBody->cmdData.openDir.handle;
      *handle = shim.response.openDir.handle;
      fsCmdBlockSetResult(blockBody, result);
      break;
   }
   case FSACommand::ReadDir:
   {
      auto entry = blockBody->cmdData.readDir.entry;
      *entry = shim.response.readDir.entry;
      fsCmdBlockSetResult(blockBody, result);
      break;
   }
   case FSACommand::OpenFile:
   case FSACommand::OpenFileByStat:
   {
      *blockBody->cmdData.openFile.handle = shim.response.openFile.handle;
      fsCmdBlockSetResult(blockBody, result);
      break;
   }
   case FSACommand::GetPosFile:
   {
      auto pos = blockBody->cmdData.getPosFile.pos;
      *pos = shim.response.getPosFile.pos;
      fsCmdBlockSetResult(blockBody, result);
      break;
   }
   case FSACommand::StatFile:
   {
      auto stat = blockBody->cmdData.statFile.stat;
      *stat = shim.response.statFile.stat;
      fsCmdBlockSetResult(blockBody, result);
      break;
   }
   case FSACommand::GetFileBlockAddress:
   {
      auto address = blockBody->cmdData.getFileBlockAddress.address;

      if (address) {
         *address = shim.response.getFileBlockAddress.address;
      }

      fsCmdBlockSetResult(blockBody, result);
      break;
   }
   case FSACommand::GetCwd:
   {
      auto bytes = blockBody->cmdData.getCwd.bytes;
      auto returnedPath = blockBody->cmdData.getCwd.returnedPath;

      if (bytes) {
         auto path = virt_addrof(shim.response.getCwd.path);
         auto len = static_cast<uint32_t>(std::strlen(path.getRawPointer()));
         decaf_check(len < bytes);
         std::strncpy(returnedPath.getRawPointer(), path.getRawPointer(), bytes);
         std::memset(returnedPath.getRawPointer() + len, 0, bytes - len);
      }

      fsCmdBlockSetResult(blockBody, result);
      break;
   }
   case FSACommand::GetInfoByQuery:
   {
      switch (shim.request.getInfoByQuery.type) {
      case FSAQueryInfoType::FreeSpaceSize:
      {
         auto freeSpaceSize = blockBody->cmdData.getInfoByQuery.freeSpaceSize;
         *freeSpaceSize = shim.response.getInfoByQuery.freeSpaceSize;
         break;
      }
      case FSAQueryInfoType::DirSize:
      {
         auto dirSize = blockBody->cmdData.getInfoByQuery.dirSize;
         *dirSize = shim.response.getInfoByQuery.dirSize;
         break;
      }
      case FSAQueryInfoType::EntryNum:
      {
         auto entryNum = blockBody->cmdData.getInfoByQuery.entryNum;
         *entryNum = shim.response.getInfoByQuery.entryNum;
         break;
      }
      case FSAQueryInfoType::FileSystemInfo:
      {
         auto fileSystemInfo = blockBody->cmdData.getInfoByQuery.fileSystemInfo;
         *fileSystemInfo = shim.response.getInfoByQuery.fileSystemInfo;
         break;
      }
      case FSAQueryInfoType::Stat:
      {
         auto stat = blockBody->cmdData.getInfoByQuery.stat;
         *stat = shim.response.getInfoByQuery.stat;
         break;
      }
      default:
         decaf_abort(fmt::format("Unexpected QueryInfoType: {}", shim.request.getInfoByQuery.type));
      }

      fsCmdBlockSetResult(blockBody, result);
      break;
   }
   default:
      decaf_abort(fmt::format("Invalid FSA command {}", shim.command));
   }
}


/**
 * Finish a FSACommand::Mount command.
 */
void
fsCmdBlockFinishMountCmd(virt_ptr<FSCmdBlockBody> blockBody,
                         FSStatus result)
{
   if (result != FSStatus::Exists) {
      fsCmdBlockFinishCmd(blockBody, result);
   } else {
      fsCmdBlockFinishCmd(blockBody, FSStatus::OK);
   }
}


/**
 * Finish a FSACommand::ReadFile command.
 *
 * Files are read in chunk of up to FSMaxBytesPerRequest bytes per time, this
 * finish function will keep requeuing the command until we have completed
 * the full read.
 */
void
fsCmdBlockFinishReadCmd(virt_ptr<FSCmdBlockBody> blockBody,
                        FSStatus result)
{
   auto bytesRead = static_cast<uint32_t>(result);

   if (result < 0) {
      return fsCmdBlockFinishCmd(blockBody, result);
   }

   // Update read state
   auto &readState = blockBody->cmdData.readFile;
   readState.bytesRead += bytesRead;
   readState.bytesRemaining -= bytesRead;

   // Check if the read is complete
   if (readState.bytesRemaining == 0 || bytesRead < readState.readSize) {
      auto chunksRead = readState.bytesRead / readState.chunkSize;
      return fsCmdBlockFinishCmd(blockBody, static_cast<FSStatus>(chunksRead));
   }

   // Check if we can read the final chunk yet
   if (readState.bytesRemaining > FSMaxBytesPerRequest) {
      readState.readSize = FSMaxBytesPerRequest;
   } else {
      readState.readSize = readState.bytesRemaining;
   }

   // Queue a new read request
   auto &readRequest = blockBody->fsaShimBuffer.request.readFile;
   readRequest.buffer = readRequest.buffer + bytesRead;
   readRequest.size = readState.readSize;
   readRequest.count = 1u;

   if (readRequest.readFlags & FSReadFlag::ReadWithPos) {
      readRequest.pos += bytesRead;
   }

   auto &shim = blockBody->fsaShimBuffer;
   shim.ioctlvVec[0].vaddr = virt_cast<virt_addr>(virt_addrof(shim.request));

   shim.ioctlvVec[1].vaddr = virt_cast<virt_addr>(readRequest.buffer);
   shim.ioctlvVec[1].len = readRequest.size;

   shim.ioctlvVec[2].vaddr = virt_cast<virt_addr>(virt_addrof(shim.response));
   fsCmdBlockRequeue(virt_addrof(blockBody->clientBody->cmdQueue),
                     blockBody, FALSE,
                     internal::FinishReadCmd);
}


/**
 * Finish a FSACommand::WriteFile command.
 *
 * Files are written in chunks of up to FSMaxBytesPerRequest bytes per time,
 * this finish function will keep requeuing the command until we have completed
 * the full write.
 */
void
fsCmdBlockFinishWriteCmd(virt_ptr<FSCmdBlockBody> blockBody,
                         FSStatus result)
{
   auto bytesWritten = static_cast<uint32_t>(result);

   if (result < 0) {
      return fsCmdBlockFinishCmd(blockBody, result);
   }

   // Update write state
   auto &writeState = blockBody->cmdData.writeFile;
   writeState.bytesWritten += bytesWritten;
   writeState.bytesRemaining -= bytesWritten;

   // Check if the write is complete
   if (writeState.bytesRemaining == 0 || bytesWritten < writeState.writeSize) {
      auto chunksWritten = writeState.bytesWritten / writeState.chunkSize;
      return fsCmdBlockFinishCmd(blockBody, static_cast<FSStatus>(chunksWritten));
   }

   // Check if we can write the final chunk yet
   if (writeState.bytesRemaining > FSMaxBytesPerRequest) {
      writeState.writeSize = FSMaxBytesPerRequest;
   } else {
      writeState.writeSize = writeState.bytesRemaining;
   }

   // Queue a new write request
   auto &writeRequest = blockBody->fsaShimBuffer.request.writeFile;
   writeRequest.buffer = writeRequest.buffer + bytesWritten;
   writeRequest.size = 1u;
   writeRequest.count = writeState.writeSize;

   if (writeRequest.writeFlags & FSWriteFlag::WriteWithPos) {
      writeRequest.pos += bytesWritten;
   }

   auto &shim = blockBody->fsaShimBuffer;
   shim.ioctlvVec[0].vaddr = virt_cast<virt_addr>(virt_addrof(shim.request));

   shim.ioctlvVec[1].vaddr = virt_cast<virt_addr>(writeRequest.buffer);
   shim.ioctlvVec[1].len = writeRequest.size;

   shim.ioctlvVec[2].vaddr = virt_cast<virt_addr>(virt_addrof(shim.response));
   fsCmdBlockRequeue(virt_addrof(blockBody->clientBody->cmdQueue),
                     blockBody, FALSE,
                     internal::FinishWriteCmd);
}


static void
fsCmdBlockFinishGetMountSourceNextOpenCmd(virt_ptr<FSCmdBlockBody> blockBody,
                                          FSStatus result)
{
   auto clientBody = blockBody->clientBody;

   if (result != FSStatus::OK) {
      return fsCmdBlockFinishCmd(blockBody, result);
   }

   auto &cmdData = blockBody->cmdData.getMountSourceNext;

   if (cmdData.dirHandle == -1) {
      auto response = virt_addrof(blockBody->fsaShimBuffer.response.openDir);
      cmdData.dirHandle = response->handle;
   }

   fsaShimPrepareRequestReadDir(virt_addrof(blockBody->fsaShimBuffer),
                                clientBody->clientHandle,
                                cmdData.dirHandle);

   fsCmdBlockRequeue(virt_addrof(blockBody->clientBody->cmdQueue),
                     blockBody, TRUE,
                     internal::FinishGetMountSourceNextReadCmd);
}


static void
fsCmdBlockFinishGetMountSourceNextReadCmd(virt_ptr<FSCmdBlockBody> blockBody,
                                          FSStatus result)
{
   auto clientBody = blockBody->clientBody;
   auto &cmdData = blockBody->cmdData.getMountSourceNext;
   cmdData.readError = result;

   if (result == FSStatus::OK) {
      auto mountSourceType = FSMountSourceType::Invalid;
      auto deviceName = virt_addrof(blockBody->fsaShimBuffer.response.readDir.entry.name);

      // Check the mount source type
      if (std::strncmp(deviceName.getRawPointer(), "sdcard", 6) == 0) {
         mountSourceType = FSMountSourceType::SdCard;
      } else if (std::strncmp(deviceName.getRawPointer(), "hfio", 4) == 0) {
         mountSourceType = FSMountSourceType::HostFileIO;
      }

      if (mountSourceType != clientBody->findMountSourceType) {
         return fsCmdBlockFinishGetMountSourceNextOpenCmd(blockBody, FSStatus::OK);
      }

      /* Note that this strncmp will rely on the fact that open / read dir
       * returns results in a consistent alphabetical order. If that is not the
       * case then this comparison becomes unreliable.
       *
       * FIXME: Unfortunately that is indeed not the case with our filesystem,
       * but fuck it because we will ?never? have more than 1 sdcard and 1 hfio.
       */
      if (std::strncmp(deviceName.getRawPointer(),
                       virt_addrof(clientBody->lastMountSourceDevice).getRawPointer(),
                       0x10) <= 0) {
         // Already returned this device, get the next one!
         return fsCmdBlockFinishGetMountSourceNextOpenCmd(blockBody,
                                                          FSStatus::OK);
      }

      // Write to FSMountSource output
      auto mountSource = cmdData.source;
      mountSource->sourceType = mountSourceType;

      if (mountSourceType == FSMountSourceType::SdCard) {
         // Map sdcardXX -> externalXX
         std::strncpy(virt_addrof(mountSource->path).getRawPointer(),
                      "external",
                      8);

         std::strncpy(virt_addrof(mountSource->path).getRawPointer() + 8,
                      deviceName.getRawPointer() + 6,
                      2);

         mountSource->path[10] = char { 0 };
      } else if (mountSourceType == FSMountSourceType::HostFileIO) {
         std::strcpy(virt_addrof(mountSource->path).getRawPointer(),
                     deviceName.getRawPointer());
      }
   }

   fsaShimPrepareRequestCloseDir(virt_addrof(blockBody->fsaShimBuffer),
                                 clientBody->clientHandle,
                                 cmdData.dirHandle);

   fsCmdBlockRequeue(virt_addrof(blockBody->clientBody->cmdQueue),
                     blockBody,
                     TRUE,
                     internal::FinishGetMountSourceNextCloseCmd);
}


static void
fsCmdBlockFinishGetMountSourceNextCloseCmd(virt_ptr<FSCmdBlockBody> blockBody,
                                           FSStatus result)
{
   fsCmdBlockFinishCmd(blockBody,
                       blockBody->cmdData.getMountSourceNext.readError);
}

} // namespace internal

void
Library::registerFsCmdBlockSymbols()
{
   RegisterFunctionExport(FSInitCmdBlock);
   RegisterFunctionExport(FSGetCmdPriority);
   RegisterFunctionExport(FSSetCmdPriority);
   RegisterFunctionExport(FSGetFSMessage);
   RegisterFunctionExport(FSGetUserData);
   RegisterFunctionExport(FSSetUserData);

   RegisterFunctionInternal(internal::fsCmdBlockFinishCmd, internal::FinishCmd);
   RegisterFunctionInternal(internal::fsCmdBlockFinishMountCmd, internal::FinishMountCmd);
   RegisterFunctionInternal(internal::fsCmdBlockFinishReadCmd, internal::FinishReadCmd);
   RegisterFunctionInternal(internal::fsCmdBlockFinishWriteCmd, internal::FinishWriteCmd);
   RegisterFunctionInternal(internal::fsCmdBlockFinishGetMountSourceNextOpenCmd, internal::FinishGetMountSourceNextOpenCmd);
   RegisterFunctionInternal(internal::fsCmdBlockFinishGetMountSourceNextReadCmd, internal::FinishGetMountSourceNextReadCmd);
   RegisterFunctionInternal(internal::fsCmdBlockFinishGetMountSourceNextCloseCmd, internal::FinishGetMountSourceNextCloseCmd);
}

} // namespace cafe::coreinit

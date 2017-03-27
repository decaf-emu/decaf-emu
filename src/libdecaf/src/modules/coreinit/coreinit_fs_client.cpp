#include "coreinit.h"
#include "coreinit_appio.h"
#include "coreinit_fs.h"
#include "coreinit_fs_client.h"
#include "coreinit_fs_driver.h"
#include "coreinit_fs_cmdblock.h"
#include "coreinit_fsa_shim.h"

#include <common/align.h>
#include <common/log.h>

namespace coreinit
{

static FSCmdQueueHandlerFn
sHandleDequeuedCommand = nullptr;

static IOSAsyncCallbackFn
sHandleFsaAsyncCallback = nullptr;


/**
 * Create a new FSClient.
 */
FSStatus
FSAddClient(FSClient *client,
            FSErrorFlag errorMask)
{
   return FSAddClientEx(client, nullptr, errorMask);
}


/**
 * Create a new FSClient.
 */
FSStatus
FSAddClientEx(FSClient *client,
              FSAttachParams *attachParams,
              FSErrorFlag errorMask)
{
   if (!internal::fsInitialised()) {
      return FSStatus::FatalError;
   }

   if (!client) {
      return FSStatus::FatalError;
   }

   if (internal::fsClientRegistered(client)) {
      internal::fsClientHandleFatalError(internal::fsClientGetBody(client), FSAStatus::AlreadyExists);
      return FSStatus::FatalError;
   }

   std::memset(client, 0, sizeof(FSClient));
   auto clientBody = internal::fsClientGetBody(client);
   auto clientHandle = internal::fsaShimOpen();

   if (clientHandle < 0) {
      return FSStatus::FatalError;
   }

   if (!internal::fsRegisterClient(clientBody)) {
      internal::fsaShimClose(clientHandle);
      internal::fsClientHandleFatalError(clientBody, FSAStatus::MaxClients);
      return FSStatus::FatalError;
   }

   internal::fsCmdQueueCreate(&clientBody->cmdQueue, sHandleDequeuedCommand, 1);

   OSFastMutex_Init(&clientBody->mutex, nullptr);
   OSCreateAlarm(&clientBody->fsmAlarm);

   internal::fsmInit(&clientBody->fsm, clientBody);

   clientBody->clientHandle = clientHandle;
   clientBody->lastDequeuedCommand = nullptr;
   clientBody->emulatedError = FSAStatus::OK;
   clientBody->unk0x14CC = 0;

   // TODO: Initialise attach params related data
   if (attachParams) {
      // 0x1240 = 4

      // 0x1248 = attachParams.callback
      // 0x124C = this

      // 0x1254 = &(this + 0x1248) // FSMessage ?
      // 0x1260 = 0xA // OSFunctionType::FsAttach ?

      // 0x1428 = 0

      // 0x1440 = attachParams.context
   } else {
      // 0x1240 = 0
      // 0x1428 = 0
   }

   return FSStatus::OK;
}


/**
 * Destroy an FSClient.
 *
 * Might block thread to wait for last command to finish.
 */
FSStatus
FSDelClient(FSClient *client,
            FSErrorFlag errorMask)
{
   auto clientBody = internal::fsClientGetBody(client);

   if (!internal::fsInitialised()) {
      return FSStatus::FatalError;
   }

   if (!clientBody->link.prev) {
      // Already deleted.
      return FSStatus::FatalError;
   }

   // Prevent any new commands from being started.
   internal::fsCmdQueueSuspend(&clientBody->cmdQueue);

   // Wait for last active command to be completed.
   auto sleptMS = 0;
   auto timeoutMS = 10;

   while (clientBody->cmdQueue.activeCmds) {
      if (clientBody->fsm.clientVolumeState != FSVolumeState::Fatal) {
         // The FS client is in an error state.
         break;
      }

      if (clientBody->lastDequeuedCommand &&
          clientBody->lastDequeuedCommand->status == FSCmdBlockStatus::Completed) {
         // The last command has completed.
         break;
      }

      if (sleptMS == timeoutMS) {
         // We have timed out waiting for last command to complete.
         break;
      }

      OSSleepTicks(internal::msToTicks(1));
      sleptMS += 1;
   }

   // Cleanup.
   internal::fsCmdQueueDestroy(&clientBody->cmdQueue);
   internal::fsaShimClose(clientBody->clientHandle);
   internal::fsDeregisterClient(clientBody);
   return FSStatus::OK;
}


/**
 * Get the current active command block for a client.
 *
 * This is the command which has been sent over IOS IPC to the FSA device.
 *
 * \return
 * Returns a pointer to FSCmdBlock or nullptr if there is no active command.
 */
FSCmdBlock *
FSGetCurrentCmdBlock(FSClient *client)
{
   auto clientBody = internal::fsClientGetBody(client);
   if (!clientBody) {
      return nullptr;
   }

   auto lastDequeued = clientBody->lastDequeuedCommand;
   if (!lastDequeued) {
      return nullptr;
   }

   return lastDequeued->cmdBlock;
}


/**
 * Get the emulated error for a client.
 *
 * \return
 * Emulated error code or a positive value on error.
 */
FSAStatus
FSGetEmulatedError(FSClient *client)
{
   auto clientBody = internal::fsClientGetBody(client);
   if (!clientBody) {
      return static_cast<FSAStatus>(1);
   }

   return clientBody->emulatedError;
}


/**
 * Set an emulated error for a client.
 *
 * All subsequent commands will fail with this error until it is cleared with
 * FSSetEmulatedError(FSAStatus::OK).
 *
 * \retval FSStatus::OK
 * Returned on success.
 *
 * \retval FSStatus::FatalError
 * Returned on failure, returned if invalid client or error code.
 */
FSStatus
FSSetEmulatedError(FSClient *client,
                   FSAStatus error)
{
   auto clientBody = internal::fsClientGetBody(client);
   if (!clientBody) {
      return FSStatus::FatalError;
   }

   if (error >= FSAStatus::OK) {
      return FSStatus::FatalError;
   }

   if (error == FSAStatus::MediaNotReady) {
      return FSStatus::FatalError;
   }

   clientBody->emulatedError = error;
   return FSStatus::OK;
}


/**
 * Get last error for a cmd as an error code for the ErrEula error viewer.
 */
int32_t
FSGetErrorCodeForViewer(FSClient *client,
                        FSCmdBlock *block)
{
   auto blockBody = internal::fsCmdBlockGetBody(block);
   auto clientBody = internal::fsClientGetBody(client);

   if (!blockBody) {
      if (clientBody->fsm.clientVolumeState == FSVolumeState::Fatal) {
         return static_cast<FSAStatus>(FSStatus::FatalError);
      } else {
         return static_cast<FSAStatus>(FSStatus::OK);
      }
   }

   if (blockBody->iosError >= 0) {
      return static_cast<FSAStatus>(FSStatus::OK);
   } else {
      // TODO: Translate error block->unk0x9f4 for FSGetErrorCodeForViewer
      return blockBody->iosError;
   }
}


/**
 * Get last error as an error code for the ErrEula error viewer.
 */
int32_t
FSGetLastErrorCodeForViewer(FSClient *client)
{
   return FSGetErrorCodeForViewer(client, FSGetCurrentCmdBlock(client));
}


/**
 * Get the error code for the last executed command.
 */
FSAStatus
FSGetLastError(FSClient *client)
{
   auto clientBody = internal::fsClientGetBody(client);
   if (!clientBody) {
      return static_cast<FSAStatus>(FSStatus::FatalError);
   }

   return clientBody->lastError;
}


/**
 * Get the volume state for a client.
 */
FSVolumeState
FSGetVolumeState(FSClient *client)
{
   auto clientBody = internal::fsClientGetBody(client);
   if (!clientBody) {
      return FSVolumeState::Invalid;
   }

   return clientBody->fsm.clientVolumeState;
}


namespace internal
{

static void
fsClientHandleFsaAsyncCallback(IOSError error,
                               void *context);

static BOOL
fsClientHandleDequeuedCommand(FSCmdBlockBody *blockBody);


/**
 * Get an aligned FSClientBody from an FSClient.
 */
FSClientBody *
fsClientGetBody(FSClient *client)
{
   auto body = mem::translate<FSClientBody>(align_up(mem::untranslate(client), 0x40));
   body->client = client;
   return body;
}


/**
 * Handle a fatal error.
 *
 * Will transition the volume state to fatal.
 */
void
fsClientHandleFatalError(FSClientBody *clientBody,
                         FSAStatus error)
{
   clientBody->lastError = error;
   clientBody->isLastErrorWithoutVolume = FALSE;
   fsmEnterState(&clientBody->fsm, FSVolumeState::Fatal, clientBody);
}


/**
 * Handle error returned from a fsaShimpPrepare* call.
 *
 * \return
 * FSStatus error code.
 */
FSStatus
fsClientHandleShimPrepareError(FSClientBody *clientBody,
                               FSAStatus error)
{
   fsClientHandleFatalError(clientBody, error);
   return fsDecodeFsaStatusToFsStatus(error);
}


/**
 * Handle async result for a synchronous FS call.
 *
 * May block and wait for result to be received.
 *
 * \return
 * Returns postive value on success, FSStatus error code otherwise.
 */
FSStatus
fsClientHandleAsyncResult(FSClient *client,
                          FSCmdBlock *block,
                          FSStatus result,
                          FSErrorFlag errorMask)
{
   auto errorFlags = FSErrorFlag::All;

   if (result >= 0) {
      auto message = OSMessage {};
      auto blockBody = fsCmdBlockGetBody(block);
      OSReceiveMessage(&blockBody->syncQueue, &message, OSMessageFlags::Blocking);

      auto fsMessage = reinterpret_cast<FSMessage *>(&message);
      if (fsMessage->type != OSFunctionType::FsCmdAsync) {
         decaf_abort(fmt::format("Unsupported function type {}", fsMessage->type));
      }

      return FSGetAsyncResult(&message)->status;
   }

   switch (result) {
   case FSStatus::Cancelled:
   case FSStatus::End:
      errorFlags = FSErrorFlag::None;
      break;
   case FSStatus::Max:
      errorFlags = FSErrorFlag::Max;
      break;
   case FSStatus::AlreadyOpen:
      errorFlags = FSErrorFlag::AlreadyOpen;
      break;
   case FSStatus::Exists:
      errorFlags = FSErrorFlag::Exists;
      break;
   case FSStatus::NotFound:
      errorFlags = FSErrorFlag::NotFound;
      break;
   case FSStatus::NotFile:
      errorFlags = FSErrorFlag::NotFile;
      break;
   case FSStatus::NotDirectory:
      errorFlags = FSErrorFlag::NotDir;
      break;
   case FSStatus::AccessError:
      errorFlags = FSErrorFlag::AccessError;
      break;
   case FSStatus::PermissionError:
      errorFlags = FSErrorFlag::PermissionError;
      break;
   case FSStatus::FileTooBig:
      errorFlags = FSErrorFlag::FileTooBig;
      break;
   case FSStatus::StorageFull:
      errorFlags = FSErrorFlag::StorageFull;
      break;
   case FSStatus::JournalFull:
      errorFlags = FSErrorFlag::JournalFull;
      break;
   case FSStatus::UnsupportedCmd:
      errorFlags = FSErrorFlag::UnsupportedCmd;
      break;
   }

   if (errorFlags != FSErrorFlag::None && (errorFlags & errorMask) == 0) {
      auto clientBody = fsClientGetBody(client);
      fsClientHandleFatalError(clientBody, clientBody->lastError);
      return FSStatus::FatalError;
   }

   return result;
}


/**
 * Submit an FSCmdBlockBody to the client's FSCmdQueue.
 */
void
fsClientSubmitCommand(FSClientBody *clientBody,
                      FSCmdBlockBody *blockBody,
                      FSFinishCmdFn finishCmdFn)
{
   auto queue = &clientBody->cmdQueue;
   blockBody->finishCmdFn = finishCmdFn;
   blockBody->status = FSCmdBlockStatus::QueuedCommand;

   // Enqueue command
   OSFastMutex_Lock(&queue->mutex);
   fsCmdQueueEnqueue(queue, blockBody, false);
   OSFastMutex_Unlock(&queue->mutex);

   // Process command queue
   fsCmdQueueProcessCmd(queue);
}


/**
 * Handle the async IOS FSA IPC callback.
 */
void
fsClientHandleFsaAsyncCallback(IOSError error,
                               void *context)
{
   auto blockBody = reinterpret_cast<FSCmdBlockBody *>(context);
   auto clientBody = blockBody->clientBody;
   blockBody->iosError = error;
   blockBody->status = FSCmdBlockStatus::Completed;
   blockBody->fsaStatus = FSAShimDecodeIosErrorToFsaStatus(clientBody->clientHandle, error);

   if (fsInitialised() && !fsDriverDone()) {
      clientBody->fsCmdHandlerMsg.data = blockBody;
      clientBody->fsCmdHandlerMsg.type = OSFunctionType::FsCmdHandler;
      OSSendMessage(OSGetDefaultAppIOQueue(), reinterpret_cast<OSMessage *>(&clientBody->fsCmdHandlerMsg), OSMessageFlags::None);
   }
}


/**
 * Handle a FS command which has been dequeued from FSCmdQueue.
 *
 * Submits the IOS FSA IPC request for the FS command.
 *
 * \retval TRUE
 * Success.
 *
 * \retval FALSE
 * Unexpected error occurred.
 */
BOOL
fsClientHandleDequeuedCommand(FSCmdBlockBody *blockBody)
{
   auto clientBody = blockBody->clientBody;
   FSAStatus error;

   do {
      /*
      if (blockBody->cmdData.mount.unk0x00 < 2) {
         if (blockBody->fsaShimBuffer.command == FSACommand::Mount) {
            // TODO: __handleDequeuedCmd FSACommand::Mount
         } else if (blockBody->fsaShimBuffer.command == FSACommand::Unmount) {
            // TODO: __handleDequeuedCmd FSACommand::Unmount
         }
      }
      */

      if (!fsInitialised() || fsDriverDone()) {
         error = FSAStatus::NotInit;
      } else {
         error = fsaShimSubmitRequestAsync(&blockBody->fsaShimBuffer,
                                           clientBody->emulatedError,
                                           sHandleFsaAsyncCallback,
                                           blockBody);
      }

      // TODO: more if cmd == mount shit

      if (error == FSAStatus::OK) {
         return TRUE;
      } else if (error == FSAStatus::NotInit) {
         gLog->error("Could not issue command {} due to uninitialised filesystem", blockBody->fsaShimBuffer.command);
         return TRUE;
      }
   } while (error == FSAStatus::Busy);

   gLog->error("Unexpected error {} whilst handling dequeued command {}", error, blockBody->fsaShimBuffer.command);
   fsmEnterState(&clientBody->fsm, FSVolumeState::Fatal, clientBody);
   return FALSE;
}

} // namespace internal

void
Module::registerFsClientFunctions()
{
   RegisterKernelFunction(FSAddClient);
   RegisterKernelFunction(FSAddClientEx);
   RegisterKernelFunction(FSDelClient);
   RegisterKernelFunction(FSGetCurrentCmdBlock);
   RegisterKernelFunction(FSGetEmulatedError);
   RegisterKernelFunction(FSSetEmulatedError);
   RegisterKernelFunction(FSGetErrorCodeForViewer);
   RegisterKernelFunction(FSGetLastErrorCodeForViewer);
   RegisterKernelFunction(FSGetLastError);
   RegisterKernelFunction(FSGetVolumeState);

   RegisterInternalFunction(internal::fsClientHandleFsaAsyncCallback, sHandleFsaAsyncCallback);
   RegisterInternalFunction(internal::fsClientHandleDequeuedCommand, sHandleDequeuedCommand);
}

} // namespace coreinit

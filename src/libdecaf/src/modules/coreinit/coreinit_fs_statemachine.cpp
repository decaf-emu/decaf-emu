#include "coreinit.h"
#include "coreinit_appio.h"
#include "coreinit_fs_statemachine.h"
#include "coreinit_fs_client.h"
#include "coreinit_fs_cmdblock.h"

#include <common/decaf_assert.h>
#include <common/log.h>
#include <spdlog/fmt/fmt.h>

namespace coreinit
{

static AlarmCallback
sFsmAlarmCallback = nullptr;


/**
 * Get an FSStateChangeInfo from an OSMessage.
 */
FSStateChangeInfo *
FSGetStateChangeInfo(OSMessage *message)
{
   return reinterpret_cast<FSStateChangeInfo *>(message->message.get());
}


/**
 * Register a callback or message queue for state change notifications.
 */
void
FSSetStateChangeNotification(FSClient *client,
                             FSStateChangeAsync *asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto fsm = &clientBody->fsm;

   if (!asyncData) {
      fsm->sendStateChangeNotifications = FALSE;
   } else {
      if (asyncData->userCallback) {
         asyncData->ioMsgQueue = OSGetDefaultAppIOQueue();
      }

      fsm->stateChangeInfo.asyncData = *asyncData;
      fsm->stateChangeInfo.msg.data = reinterpret_cast<void *>(&fsm->stateChangeInfo);
      fsm->stateChangeInfo.msg.type = OSFunctionType::FsStateChangeEvent;
      fsm->stateChangeInfo.client = clientBody->client;
      fsm->sendStateChangeNotifications = TRUE;
   }
}


namespace internal
{

static FSVolumeState
fsmOnStateChange(FSFsm *fsm,
                 FSVolumeState state,
                 FSClientBody *clientBody);

static FSVolumeState
fsmOnStateChangeFromInitial(FSFsm *fsm,
                            FSVolumeState state,
                            FSClientBody *clientBody);

static FSVolumeState
fsmOnStateChangeFromReady(FSFsm *fsm,
                          FSVolumeState state,
                          FSClientBody *clientBody);

static FSVolumeState
fsmOnStateChangeFromNoMedia(FSFsm *fsm,
                            FSVolumeState state,
                            FSClientBody *clientBody);

static FSVolumeState
fsmOnStateChangeFromMediaError(FSFsm *fsm,
                               FSVolumeState state,
                               FSClientBody *clientBody);

static FSVolumeState
fsmOnStateChangeFromFatal(FSFsm *fsm,
                          FSVolumeState state,
                          FSClientBody *clientBody);

static void
fsmNotifyStateChange(FSFsm *fsm);

static void
fsmStartAlarm(FSClientBody *clientBody);

static void
fsmAlarmHandler(OSAlarm *alarm, OSContext *context);


/**
 * Initialise the FS State Machine.
 */
void
fsmInit(FSFsm *fsm,
        FSClientBody *clientBody)
{
   OSFastMutex_Lock(&clientBody->mutex);
   fsm->state = FSVolumeState::Initial;
   fsm->unk0x0c = FALSE;

   internal::fsmEnterState(fsm, FSVolumeState::Ready, clientBody);
   fsm->clientVolumeState = fsm->state;

   OSFastMutex_Unlock(&clientBody->mutex);
}


/**
 * Set the FSM state.
 */
void
fsmSetState(FSFsm *fsm, FSVolumeState state, FSClientBody *clientBody)
{
   OSFastMutex_Lock(&clientBody->mutex);

   // Set the state
   auto newState = fsmOnStateChange(fsm, state, clientBody);

   // Handle the transition
   fsmEnterState(fsm, newState, clientBody);

   OSFastMutex_Unlock(&clientBody->mutex);
}


/**
 * Enter an FSM state.
 */
void
fsmEnterState(FSFsm *fsm,
              FSVolumeState state,
              FSClientBody *clientBody)
{
   OSFastMutex_Lock(&clientBody->mutex);

   while (state != FSVolumeState::Invalid) {
      fsmOnStateChange(fsm, FSVolumeState::NoMedia, clientBody);
      fsm->state = state;
      state = fsmOnStateChange(fsm, FSVolumeState::Ready, clientBody);
   }

   if (fsm->unk0x0c) {
      fsm->unk0x0c = FALSE;

      if (fsm->state != fsm->clientVolumeState) {
         fsm->clientVolumeState = fsm->state;
         gLog->error("Updated volume state of client 0x{:X} to {}", clientBody->client.getAddress(), fsm->clientVolumeState);

         if (fsm->clientVolumeState == FSVolumeState::Fatal || fsm->clientVolumeState == FSVolumeState::JournalFull) {
            gLog->error("Shit has become fucked {}", clientBody->lastError);
         }

         fsmNotifyStateChange(fsm);
      }
   }

   OSFastMutex_Unlock(&clientBody->mutex);
}


/**
 * Send a state change notification message.
 */
void
fsmNotifyStateChange(FSFsm *fsm)
{
   if (fsm->sendStateChangeNotifications) {
      fsm->stateChangeInfo.state = fsm->state;

      OSSendMessage(fsm->stateChangeInfo.asyncData.ioMsgQueue,
                    reinterpret_cast<OSMessage *>(&fsm->stateChangeInfo.msg),
                    OSMessageFlags::None);
   }
}


/**
 * Start the FSM alarm.
 *
 * I don't really understand it's purpose.
 */
void
fsmStartAlarm(FSClientBody *clientBody)
{
   OSCancelAlarm(&clientBody->fsmAlarm);
   OSCreateAlarm(&clientBody->fsmAlarm);
   OSSetAlarmUserData(&clientBody->fsmAlarm, clientBody);
   OSSetAlarm(&clientBody->fsmAlarm, internal::msToTicks(1000), sFsmAlarmCallback);
}


/**
 * Alarm handler for the FSM alarm.
 *
 * Does things.
 */
void
fsmAlarmHandler(OSAlarm *alarm, OSContext *context)
{
   auto clientBody = reinterpret_cast<FSClientBody *>(OSGetAlarmUserData(alarm));
   OSFastMutex_Lock(&clientBody->mutex);

   if (clientBody->fsm.unk0x08 == FSVolumeState::Invalid) {
      clientBody->fsm.unk0x08 = FSVolumeState::NoMedia;
      fsmSetState(&clientBody->fsm, FSVolumeState::NoMedia, clientBody);
   } else if (clientBody->fsm.unk0x08 == FSVolumeState::WrongMedia) {
      fsmSetState(&clientBody->fsm, FSVolumeState::InvalidMedia, clientBody);
   }

   OSFastMutex_Unlock(&clientBody->mutex);
}


/**
 * Called when the FSM state has changed.
 *
 * \return
 * Returns the next state.
 */
FSVolumeState
fsmOnStateChange(FSFsm *fsm,
                 FSVolumeState state,
                 FSClientBody *clientBody)
{
   switch (fsm->state) {
   case FSVolumeState::Initial:
      return fsmOnStateChangeFromInitial(fsm, state, clientBody);
   case FSVolumeState::Ready:
      return fsmOnStateChangeFromReady(fsm, state, clientBody);
   case FSVolumeState::NoMedia:
      return fsmOnStateChangeFromNoMedia(fsm, state, clientBody);
   case FSVolumeState::InvalidMedia:
   case FSVolumeState::DirtyMedia:
   case FSVolumeState::WrongMedia:
   case FSVolumeState::MediaError:
   case FSVolumeState::DataCorrupted:
   case FSVolumeState::WriteProtected:
      return fsmOnStateChangeFromMediaError(fsm, state, clientBody);
   case FSVolumeState::JournalFull:
   case FSVolumeState::Fatal:
      return fsmOnStateChangeFromFatal(fsm, state, clientBody);
   default:
      decaf_abort(fmt::format("Invalid FSM state transition from {} to {}!", fsm->state, state));
   }

   return FSVolumeState::Invalid;
}


/**
 * Called when the FSM state has changed from FSVolumeState::Initial.
 *
 * \return
 * Returns the next state.
 */
FSVolumeState
fsmOnStateChangeFromInitial(FSFsm *fsm,
                            FSVolumeState state,
                            FSClientBody *clientBody)
{
   switch (state) {
   case FSVolumeState::NoMedia:
      return FSVolumeState::Invalid;
   default:
      decaf_abort(fmt::format("Invalid FSM state transition from {} to {}!", fsm->state, state));
   }

   return FSVolumeState::Invalid;
}


/**
 * Called when the FSM state has changed from FSVolumeState::Ready.
 *
 * \return
 * Returns the next state.
 */
FSVolumeState
fsmOnStateChangeFromReady(FSFsm *fsm,
                          FSVolumeState state,
                          FSClientBody *clientBody)
{
   switch (state) {
   case FSVolumeState::Ready:
   {
      if (clientBody->lastDequeuedCommand) {
         fsCmdBlockRequeue(&clientBody->cmdQueue,
                           clientBody->lastDequeuedCommand,
                           TRUE,
                           clientBody->lastDequeuedCommand->finishCmdFn);
      }

      return FSVolumeState::Invalid;
   }
   case FSVolumeState::WrongMedia:
   {
      clientBody->fsm.unk0x08 = FSVolumeState::Invalid;
      fsmStartAlarm(clientBody);
      return FSVolumeState::NoMedia;
   }
   case FSVolumeState::MediaError:
      return FSVolumeState::WriteProtected;
   case FSVolumeState::DataCorrupted:
      return FSVolumeState::DataCorrupted;
   case FSVolumeState::WriteProtected:
      return FSVolumeState::MediaError;
   case FSVolumeState::NoMedia:
   case FSVolumeState::Invalid:
      return FSVolumeState::Invalid;
   default:
      decaf_abort(fmt::format("Invalid FSM state transition from {} to {}!", fsm->state, state));
   }

   return FSVolumeState::Invalid;
}


/**
 * Called when the FSM state has changed from FSVolumeState::NoMedia.
 *
 * \return
 * Returns the next state.
 */
FSVolumeState
fsmOnStateChangeFromNoMedia(FSFsm *fsm,
                            FSVolumeState state,
                            FSClientBody *clientBody)
{
   switch (fsm->state) {
   case FSVolumeState::Ready:
   case FSVolumeState::Fatal:
   {
      return FSVolumeState::Invalid;
   }
   case FSVolumeState::NoMedia:
   {
      OSCancelAlarm(&clientBody->fsmAlarm);
      fsm->unk0x0c = TRUE;
      return FSVolumeState::Invalid;
   }
   case FSVolumeState::InvalidMedia:
   {
      fsm->unk0x0c = TRUE;

      if (fsm->unk0x08 == FSVolumeState::NoMedia) {
         return FSVolumeState::Invalid;
      }

      return FSVolumeState::NoMedia;
   }
   case FSVolumeState::JournalFull:
   {
      clientBody->unk0x14D0 = 1;
      return FSVolumeState::Invalid;
   }
   case FSVolumeState::Invalid:
   {
      auto lastCmd = clientBody->lastDequeuedCommand;
      clientBody->lastDequeuedCommand = nullptr;
      lastCmd->cancelFlags &= ~FSCmdCancelFlags::Cancelling;
      lastCmd->status = FSCmdBlockStatus::Cancelled;
      fsCmdBlockReplyResult(lastCmd, FSStatus::Cancelled);
      clientBody->unk0x14CC = 0;
      clientBody->unk0x14D0 = 1;
      fsm->unk0x0c = TRUE;
      return FSVolumeState::Ready;
   }
   default:
      decaf_abort(fmt::format("Invalid FSM state transition from {} to {}!", fsm->state, state));
   }

   return FSVolumeState::Invalid;
}


/**
 * Called when the FSM state has changed from various media error related states.
 *
 * \return
 * Returns the next state.
 */
FSVolumeState
fsmOnStateChangeFromMediaError(FSFsm *fsm,
                               FSVolumeState state,
                               FSClientBody *clientBody)
{
   switch (fsm->state) {
   case FSVolumeState::Ready:
   case FSVolumeState::NoMedia:
   {
      return FSVolumeState::Invalid;
   }
   case FSVolumeState::InvalidMedia:
   {
      clientBody->unk0x14CC = 1;
      return FSVolumeState::Invalid;
   }
   case FSVolumeState::JournalFull:
   {
      fsm->unk0x0c = TRUE;
      clientBody->unk0x14D0 = 1;
      return FSVolumeState::Invalid;
   }
   case FSVolumeState::Fatal:
   {
      fsm->unk0x0c = TRUE;
      return FSVolumeState::NoMedia;
   }
   case FSVolumeState::Invalid:
   {
      auto lastCmd = clientBody->lastDequeuedCommand;
      clientBody->lastDequeuedCommand = nullptr;
      lastCmd->cancelFlags &= ~FSCmdCancelFlags::Cancelling;
      lastCmd->status = FSCmdBlockStatus::Cancelled;
      fsCmdBlockReplyResult(lastCmd, FSStatus::Cancelled);
      clientBody->unk0x14CC = 0;
      clientBody->unk0x14D0 = 1;
      fsm->unk0x0c = TRUE;
      return FSVolumeState::Ready;
   }
   default:
      decaf_abort(fmt::format("Invalid FSM state transition from {} to {}!", fsm->state, state));
   }

   return FSVolumeState::Invalid;
}


/**
 * Called when the FSM state has changed from FSVolumeState::Fatal or
 * FSVolumeState::JournalFull.
 *
 * \return
 * Returns the next state.
 */
FSVolumeState
fsmOnStateChangeFromFatal(FSFsm *fsm,
                          FSVolumeState state,
                          FSClientBody *clientBody)
{
   switch (fsm->state) {
   case FSVolumeState::Ready:
   case FSVolumeState::JournalFull:
   {
      if (clientBody->isLastErrorWithoutVolume) {
         fsm->unk0x0c = TRUE;
      }

      return FSVolumeState::Invalid;
   }
   case FSVolumeState::NoMedia:
   case FSVolumeState::InvalidMedia:
   case FSVolumeState::DirtyMedia:
   case FSVolumeState::WrongMedia:
   case FSVolumeState::MediaError:
   case FSVolumeState::DataCorrupted:
   case FSVolumeState::WriteProtected:
   case FSVolumeState::Fatal:
   case FSVolumeState::Invalid:
      return FSVolumeState::Invalid;
   default:
      decaf_abort(fmt::format("Invalid FSM state transition from {} to {}!", fsm->state, state));
   }

   return FSVolumeState::Invalid;
}

} // namespace internal

void
Module::registerFsFsmFunctions()
{
   RegisterKernelFunction(FSGetStateChangeInfo);
   RegisterKernelFunction(FSSetStateChangeNotification);

   RegisterInternalFunction(internal::fsmAlarmHandler, sFsmAlarmCallback);
}

} // namespace coreinit

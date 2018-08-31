#include "coreinit.h"
#include "coreinit_appio.h"
#include "coreinit_fs_statemachine.h"
#include "coreinit_fs_client.h"
#include "coreinit_fs_cmdblock.h"

#include <common/decaf_assert.h>
#include <common/log.h>
#include <fmt/format.h>

namespace cafe::coreinit
{

static AlarmCallbackFn sFsmAlarmCallback = nullptr;


/**
 * Get an FSStateChangeInfo from an OSMessage.
 */
virt_ptr<FSStateChangeInfo>
FSGetStateChangeInfo(virt_ptr<OSMessage> message)
{
   return virt_cast<FSStateChangeInfo *>(message->message);
}


/**
 * Register a callback or message queue for state change notifications.
 */
void
FSSetStateChangeNotification(virt_ptr<FSClient> client,
                             virt_ptr<FSStateChangeAsync> asyncData)
{
   auto clientBody = internal::fsClientGetBody(client);
   auto &fsm = clientBody->fsm;

   if (!asyncData) {
      fsm.sendStateChangeNotifications = FALSE;
   } else {
      if (asyncData->userCallback) {
         asyncData->ioMsgQueue = OSGetDefaultAppIOQueue();
      }

      fsm.stateChangeInfo.asyncData = *asyncData;
      fsm.stateChangeInfo.msg.data = virt_addrof(fsm.stateChangeInfo);
      fsm.stateChangeInfo.msg.type = OSFunctionType::FsStateChangeEvent;
      fsm.stateChangeInfo.client = clientBody->client;
      fsm.sendStateChangeNotifications = TRUE;
   }
}


namespace internal
{

static FSVolumeState
fsmOnStateChange(virt_ptr<FSFsm> fsm,
                 FSVolumeState state,
                 virt_ptr<FSClientBody> clientBody);

static FSVolumeState
fsmOnStateChangeFromInitial(virt_ptr<FSFsm> fsm,
                            FSVolumeState state,
                            virt_ptr<FSClientBody> clientBody);

static FSVolumeState
fsmOnStateChangeFromReady(virt_ptr<FSFsm> fsm,
                          FSVolumeState state,
                          virt_ptr<FSClientBody> clientBody);

static FSVolumeState
fsmOnStateChangeFromNoMedia(virt_ptr<FSFsm> fsm,
                            FSVolumeState state,
                            virt_ptr<FSClientBody> clientBody);

static FSVolumeState
fsmOnStateChangeFromMediaError(virt_ptr<FSFsm> fsm,
                               FSVolumeState state,
                               virt_ptr<FSClientBody> clientBody);

static FSVolumeState
fsmOnStateChangeFromFatal(virt_ptr<FSFsm> fsm,
                          FSVolumeState state,
                          virt_ptr<FSClientBody> clientBody);

static void
fsmNotifyStateChange(virt_ptr<FSFsm> fsm);

static void
fsmStartAlarm(virt_ptr<FSClientBody> clientBody);

static void
fsmAlarmHandler(virt_ptr<OSAlarm> alarm,
                virt_ptr<OSContext> context);


/**
 * Initialise the FS State Machine.
 */
void
fsmInit(virt_ptr<FSFsm> fsm,
        virt_ptr<FSClientBody> clientBody)
{
   OSFastMutex_Lock(virt_addrof(clientBody->mutex));
   fsm->state = FSVolumeState::Initial;
   fsm->unk0x0c = FALSE;

   internal::fsmEnterState(fsm, FSVolumeState::Ready, clientBody);
   fsm->clientVolumeState = fsm->state;

   OSFastMutex_Unlock(virt_addrof(clientBody->mutex));
}


/**
 * Set the FSM state.
 */
void
fsmSetState(virt_ptr<FSFsm> fsm,
            FSVolumeState state,
            virt_ptr<FSClientBody> clientBody)
{
   OSFastMutex_Lock(virt_addrof(clientBody->mutex));

   // Set the state
   auto newState = fsmOnStateChange(fsm, state, clientBody);

   // Handle the transition
   fsmEnterState(fsm, newState, clientBody);

   OSFastMutex_Unlock(virt_addrof(clientBody->mutex));
}


/**
 * Enter an FSM state.
 */
void
fsmEnterState(virt_ptr<FSFsm> fsm,
              FSVolumeState state,
              virt_ptr<FSClientBody> clientBody)
{
   OSFastMutex_Lock(virt_addrof(clientBody->mutex));

   while (state != FSVolumeState::Invalid) {
      fsmOnStateChange(fsm, FSVolumeState::NoMedia, clientBody);
      fsm->state = state;
      state = fsmOnStateChange(fsm, FSVolumeState::Ready, clientBody);
   }

   if (fsm->unk0x0c) {
      fsm->unk0x0c = FALSE;

      if (fsm->state != fsm->clientVolumeState) {
         fsm->clientVolumeState = fsm->state;
         gLog->error("Updated volume state of client 0x{:X} to {}",
                     clientBody->client, fsm->clientVolumeState);

         if (fsm->clientVolumeState == FSVolumeState::Fatal ||
             fsm->clientVolumeState == FSVolumeState::JournalFull) {
            gLog->error("Shit has become fucked {}", clientBody->lastError);
         }

         fsmNotifyStateChange(fsm);
      }
   }

   OSFastMutex_Unlock(virt_addrof(clientBody->mutex));
}


/**
 * Send a state change notification message.
 */
void
fsmNotifyStateChange(virt_ptr<FSFsm> fsm)
{
   if (fsm->sendStateChangeNotifications) {
      fsm->stateChangeInfo.state = fsm->state;

      OSSendMessage(fsm->stateChangeInfo.asyncData.ioMsgQueue,
                    virt_cast<OSMessage *>(virt_addrof(fsm->stateChangeInfo.msg)),
                    OSMessageFlags::None);
   }
}


/**
 * Start the FSM alarm.
 *
 * I don't really understand it's purpose.
 */
void
fsmStartAlarm(virt_ptr<FSClientBody> clientBody)
{
   OSCancelAlarm(virt_addrof(clientBody->fsmAlarm));
   OSCreateAlarm(virt_addrof(clientBody->fsmAlarm));
   OSSetAlarmUserData(virt_addrof(clientBody->fsmAlarm), clientBody);
   OSSetAlarm(virt_addrof(clientBody->fsmAlarm),
              internal::msToTicks(1000),
              sFsmAlarmCallback);
}


/**
 * Alarm handler for the FSM alarm.
 *
 * Does things.
 */
void
fsmAlarmHandler(virt_ptr<OSAlarm> alarm,
                virt_ptr<OSContext> context)
{
   auto clientBody = virt_cast<FSClientBody *>(OSGetAlarmUserData(alarm));
   OSFastMutex_Lock(virt_addrof(clientBody->mutex));

   if (clientBody->fsm.unk0x08 == FSVolumeState::Invalid) {
      clientBody->fsm.unk0x08 = FSVolumeState::NoMedia;
      fsmSetState(virt_addrof(clientBody->fsm),
                  FSVolumeState::NoMedia,
                  clientBody);
   } else if (clientBody->fsm.unk0x08 == FSVolumeState::WrongMedia) {
      fsmSetState(virt_addrof(clientBody->fsm),
                  FSVolumeState::InvalidMedia,
                  clientBody);
   }

   OSFastMutex_Unlock(virt_addrof(clientBody->mutex));
}


/**
 * Called when the FSM state has changed.
 *
 * \return
 * Returns the next state.
 */
FSVolumeState
fsmOnStateChange(virt_ptr<FSFsm> fsm,
                 FSVolumeState state,
                 virt_ptr<FSClientBody> clientBody)
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
fsmOnStateChangeFromInitial(virt_ptr<FSFsm> fsm,
                            FSVolumeState state,
                            virt_ptr<FSClientBody> clientBody)
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
fsmOnStateChangeFromReady(virt_ptr<FSFsm> fsm,
                          FSVolumeState state,
                          virt_ptr<FSClientBody> clientBody)
{
   switch (state) {
   case FSVolumeState::Ready:
   {
      if (clientBody->lastDequeuedCommand) {
         fsCmdBlockRequeue(virt_addrof(clientBody->cmdQueue),
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
fsmOnStateChangeFromNoMedia(virt_ptr<FSFsm> fsm,
                            FSVolumeState state,
                            virt_ptr<FSClientBody> clientBody)
{
   switch (fsm->state) {
   case FSVolumeState::Ready:
   case FSVolumeState::Fatal:
   {
      return FSVolumeState::Invalid;
   }
   case FSVolumeState::NoMedia:
   {
      OSCancelAlarm(virt_addrof(clientBody->fsmAlarm));
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
      clientBody->unk0x14D0 = 1u;
      return FSVolumeState::Invalid;
   }
   case FSVolumeState::Invalid:
   {
      auto lastCmd = clientBody->lastDequeuedCommand;
      clientBody->lastDequeuedCommand = nullptr;
      lastCmd->cancelFlags &= ~FSCmdCancelFlags::Cancelling;
      lastCmd->status = FSCmdBlockStatus::Cancelled;
      fsCmdBlockReplyResult(lastCmd, FSStatus::Cancelled);
      clientBody->unk0x14CC = 0u;
      clientBody->unk0x14D0 = 1u;
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
fsmOnStateChangeFromMediaError(virt_ptr<FSFsm> fsm,
                               FSVolumeState state,
                               virt_ptr<FSClientBody> clientBody)
{
   switch (fsm->state) {
   case FSVolumeState::Ready:
   case FSVolumeState::NoMedia:
   {
      return FSVolumeState::Invalid;
   }
   case FSVolumeState::InvalidMedia:
   {
      clientBody->unk0x14CC = 1u;
      return FSVolumeState::Invalid;
   }
   case FSVolumeState::JournalFull:
   {
      fsm->unk0x0c = TRUE;
      clientBody->unk0x14D0 = 1u;
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
      clientBody->unk0x14CC = 0u;
      clientBody->unk0x14D0 = 1u;
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
fsmOnStateChangeFromFatal(virt_ptr<FSFsm> fsm,
                          FSVolumeState state,
                          virt_ptr<FSClientBody> clientBody)
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
Library::registerFsStateMachineSymbols()
{
   RegisterFunctionExport(FSGetStateChangeInfo);
   RegisterFunctionExport(FSSetStateChangeNotification);

   RegisterFunctionInternal(internal::fsmAlarmHandler, sFsmAlarmCallback);
}

} // namespace cafe::coreinit

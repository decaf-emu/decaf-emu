#pragma once
#include "coreinit_enum.h"
#include "coreinit_fs.h"
#include "coreinit_messagequeue.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/be_ptr.h>
#include <common/cbool.h>
#include <common/structsize.h>

namespace coreinit
{

/**
 * \ingroup coreinit_fs
 * @{
 */

struct FSClient;
struct FSClientBody;

using FSStateChangeCallbackFn = wfunc_ptr<void, FSClient *, FSVolumeState, void *>;


struct FSStateChangeAsync
{
   //! Callback for state change notifications.
   FSStateChangeCallbackFn::be userCallback;

   //! Context for state change notification callback.
   be_ptr<void> userContext;

   //! Message queue to insert state change message on.
   be_ptr<OSMessageQueue> ioMsgQueue;
};
CHECK_OFFSET(FSStateChangeAsync, 0x0, userCallback);
CHECK_OFFSET(FSStateChangeAsync, 0x4, userContext);
CHECK_OFFSET(FSStateChangeAsync, 0x8, ioMsgQueue);
CHECK_SIZE(FSStateChangeAsync, 0xC);


/**
 * Data returned from FSGetStateChangeInfo.
 */
struct FSStateChangeInfo
{
   //! Async data.
   FSStateChangeAsync asyncData;

   //! Message used for asyncData.ioMsgQueue.
   FSMessage msg;

   //! Client which this notification is for.
   be_ptr<FSClient> client;

   //! Volume state at the time at which the notification was sent.
   be_val<FSVolumeState> state;
};
CHECK_OFFSET(FSStateChangeInfo, 0x00, asyncData);
CHECK_OFFSET(FSStateChangeInfo, 0x0C, msg);
CHECK_OFFSET(FSStateChangeInfo, 0x1C, client);
CHECK_OFFSET(FSStateChangeInfo, 0x20, state);
CHECK_SIZE(FSStateChangeInfo, 0x24);


/**
 * FileSystem State Machine.
 *
 * I don't really understand the control flow of the FSM properly...
 */
struct FSFsm
{
   be_val<FSVolumeState> state;
   be_val<FSVolumeState> clientVolumeState;
   be_val<FSVolumeState> unk0x08;
   be_val<BOOL> unk0x0c;

   //! If TRUE then will send state change notifications.
   be_val<BOOL> sendStateChangeNotifications;

   FSStateChangeInfo stateChangeInfo;
};
CHECK_OFFSET(FSFsm, 0x0, state);
CHECK_OFFSET(FSFsm, 0x4, clientVolumeState);
CHECK_OFFSET(FSFsm, 0x8, unk0x08);
CHECK_OFFSET(FSFsm, 0xC, unk0x0c);
CHECK_OFFSET(FSFsm, 0x10, sendStateChangeNotifications);
CHECK_OFFSET(FSFsm, 0x14, stateChangeInfo);
CHECK_SIZE(FSFsm, 0x38);

FSStateChangeInfo *
FSGetStateChangeInfo(OSMessage *message);

void
FSSetStateChangeNotification(FSClient *client,
                             FSStateChangeAsync *asyncData);

namespace internal
{

void
fsmInit(FSFsm *fsm,
        FSClientBody *clientBody);

void
fsmSetState(FSFsm *fsm,
            FSVolumeState state,
            FSClientBody *clientBody);

void
fsmEnterState(FSFsm *fsm,
              FSVolumeState state,
              FSClientBody *clientBody);

} // namespace internal

/** @} */

} // namespace coreinit

#pragma once
#include "coreinit_enum.h"
#include "coreinit_fs.h"
#include "coreinit_messagequeue.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \ingroup coreinit_fs
 * @{
 */

struct FSClient;
struct FSClientBody;

using FSStateChangeCallbackFn = virt_func_ptr<void(virt_ptr<FSClient>,
                                                   FSVolumeState,
                                                   virt_ptr<void>)>;


struct FSStateChangeAsync
{
   //! Callback for state change notifications.
   be2_val<FSStateChangeCallbackFn> userCallback;

   //! Context for state change notification callback.
   be2_virt_ptr<void> userContext;

   //! Message queue to insert state change message on.
   be2_virt_ptr<OSMessageQueue> ioMsgQueue;
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
   be2_struct<FSStateChangeAsync> asyncData;

   //! Message used for asyncData.ioMsgQueue.
   be2_struct<FSMessage> msg;

   //! Client which this notification is for.
   be2_virt_ptr<FSClient> client;

   //! Volume state at the time at which the notification was sent.
   be2_val<FSVolumeState> state;
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
   be2_val<FSVolumeState> state;
   be2_val<FSVolumeState> clientVolumeState;
   be2_val<FSVolumeState> unk0x08;
   be2_val<BOOL> unk0x0c;

   //! If TRUE then will send state change notifications.
   be2_val<BOOL> sendStateChangeNotifications;

   be2_struct<FSStateChangeInfo> stateChangeInfo;
};
CHECK_OFFSET(FSFsm, 0x0, state);
CHECK_OFFSET(FSFsm, 0x4, clientVolumeState);
CHECK_OFFSET(FSFsm, 0x8, unk0x08);
CHECK_OFFSET(FSFsm, 0xC, unk0x0c);
CHECK_OFFSET(FSFsm, 0x10, sendStateChangeNotifications);
CHECK_OFFSET(FSFsm, 0x14, stateChangeInfo);
CHECK_SIZE(FSFsm, 0x38);

virt_ptr<FSStateChangeInfo>
FSGetStateChangeInfo(virt_ptr<OSMessage> message);

void
FSSetStateChangeNotification(virt_ptr<FSClient> client,
                             virt_ptr<FSStateChangeAsync> asyncData);

namespace internal
{

void
fsmInit(virt_ptr<FSFsm> fsm,
        virt_ptr<FSClientBody> clientBody);

void
fsmSetState(virt_ptr<FSFsm> fsm,
            FSVolumeState state,
            virt_ptr<FSClientBody> clientBody);

void
fsmEnterState(virt_ptr<FSFsm> fsm,
              FSVolumeState state,
              virt_ptr<FSClientBody> clientBody);

} // namespace internal

/** @} */

} // namespace cafe::coreinit

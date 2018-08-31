#pragma once
#include "coreinit_alarm.h"
#include "coreinit_enum.h"
#include "coreinit_ios.h"
#include "coreinit_fastmutex.h"
#include "coreinit_fs.h"
#include "coreinit_fs_cmdblock.h"
#include "coreinit_fs_cmdqueue.h"
#include "coreinit_fs_statemachine.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \ingroup coreinit_fs
 * @{
 */

struct FSClient;
struct FSClientBody;
struct FSClientBodyLink;
struct FSCmdBlockBody;


/**
 * Attach parameters passed to FSAddClientEx.
 */
struct FSAttachParams
{
   be2_virt_ptr<void> userCallback;
   be2_virt_ptr<void> userContext;
};
CHECK_OFFSET(FSAttachParams, 0x00, userCallback);
CHECK_OFFSET(FSAttachParams, 0x04, userContext);
CHECK_SIZE(FSAttachParams, 0x8);


/**
 * Client container, use internal::getClientBody to get the actual data.
 */
struct FSClient
{
   be2_array<uint8_t, 0x1700> data;
};
CHECK_SIZE(FSClient, 0x1700);


/**
 * Link entry used for FSClientBodyQueue.
 */
struct FSClientBodyLink
{
   be2_virt_ptr<FSClientBody> next;
   be2_virt_ptr<FSClientBody> prev;
};
CHECK_OFFSET(FSClientBodyLink, 0x00, next);
CHECK_OFFSET(FSClientBodyLink, 0x04, prev);
CHECK_SIZE(FSClientBodyLink, 0x8);


/**
 * A queue structure to contain FSClientBody.
 */
struct FSClientBodyQueue
{
   be2_virt_ptr<FSClientBody> head;
   be2_virt_ptr<FSClientBody> tail;
};
CHECK_OFFSET(FSClientBodyQueue, 0x00, head);
CHECK_OFFSET(FSClientBodyQueue, 0x04, tail);
CHECK_SIZE(FSClientBodyQueue, 0x08);


/**
 * The actual data of an FSClient.
 */
struct FSClientBody
{
   UNKNOWN(0x1444);

   //! IOSHandle returned from fsaShimOpen.
   be2_val<IOSHandle> clientHandle;

   //! State machine.
   be2_struct<FSFsm> fsm;

   //! Command queue of FS commands.
   be2_struct<FSCmdQueue> cmdQueue;

   //! The last dequeued command.
   be2_virt_ptr<FSCmdBlockBody> lastDequeuedCommand;

   //! Emulated error, set with FSSetEmulatedError.
   be2_val<FSAStatus> emulatedError;

   be2_val<uint32_t> unk0x14CC;
   be2_val<uint32_t> unk0x14D0;
   UNKNOWN(0x1560 - 0x14D4);

   //! Mutex used to protect FSClientBody data.
   be2_struct<OSFastMutex> mutex;

   UNKNOWN(4);

   //! Alarm used by fsm for unknown reasons.
   be2_struct<OSAlarm> fsmAlarm;

   //! Error of last FS command.
   be2_val<FSAStatus> lastError;

   be2_val<BOOL> isLastErrorWithoutVolume;

   //! Message used to send FsCmdHandler message when FSA async callback is received.
   be2_struct<FSMessage> fsCmdHandlerMsg;

   //! Device name of the last mount source returned by FSGetMountSourceNext.
   be2_array<char, 0x10> lastMountSourceDevice;

   //! Mount source type to find with FSGetMountSourceNext.
   be2_val<FSMountSourceType> findMountSourceType;

   //! Link used for linked list of clients.
   be2_struct<FSClientBodyLink> link;

   //! Pointer to unaligned FSClient structure.
   be2_virt_ptr<FSClient> client;
};
CHECK_OFFSET(FSClientBody, 0x1444, clientHandle);
CHECK_OFFSET(FSClientBody, 0x1448, fsm);
CHECK_OFFSET(FSClientBody, 0x1480, cmdQueue);
CHECK_OFFSET(FSClientBody, 0x14C4, lastDequeuedCommand);
CHECK_OFFSET(FSClientBody, 0x14C8, emulatedError);
CHECK_OFFSET(FSClientBody, 0x14CC, unk0x14CC);
CHECK_OFFSET(FSClientBody, 0x14D0, unk0x14D0);
CHECK_OFFSET(FSClientBody, 0x1560, mutex);
CHECK_OFFSET(FSClientBody, 0x1590, fsmAlarm);
CHECK_OFFSET(FSClientBody, 0x15E8, lastError);
CHECK_OFFSET(FSClientBody, 0x15EC, isLastErrorWithoutVolume);
CHECK_OFFSET(FSClientBody, 0x15F0, fsCmdHandlerMsg);
CHECK_OFFSET(FSClientBody, 0x1600, lastMountSourceDevice);
CHECK_OFFSET(FSClientBody, 0x1610, findMountSourceType);
CHECK_OFFSET(FSClientBody, 0x1614, link);
CHECK_OFFSET(FSClientBody, 0x161C, client);

FSStatus
FSAddClientEx(virt_ptr<FSClient> client,
              virt_ptr<FSAttachParams> attachParams,
              FSErrorFlag errorMask);

FSStatus
FSAddClient(virt_ptr<FSClient> client,
            FSErrorFlag errorMask);

FSStatus
FSDelClient(virt_ptr<FSClient> client,
            FSErrorFlag errorMask);

virt_ptr<FSCmdBlock>
FSGetCurrentCmdBlock(virt_ptr<FSClient> client);

FSAStatus
FSGetEmulatedError(virt_ptr<FSClient> client);

FSStatus
FSSetEmulatedError(virt_ptr<FSClient> client,
                   FSAStatus error);

int32_t
FSGetErrorCodeForViewer(virt_ptr<FSClient> client,
                        virt_ptr<FSCmdBlock> block);

int32_t
FSGetLastErrorCodeForViewer(virt_ptr<FSClient> client);

FSAStatus
FSGetLastError(virt_ptr<FSClient> client);

FSVolumeState
FSGetVolumeState(virt_ptr<FSClient> client);

namespace internal
{

virt_ptr<FSClientBody>
fsClientGetBody(virt_ptr<FSClient> client);

void
fsClientHandleFatalError(virt_ptr<FSClientBody> clientBody,
                         FSAStatus error);

FSStatus
fsClientHandleShimPrepareError(virt_ptr<FSClientBody> clientBody,
                               FSAStatus error);

FSStatus
fsClientHandleAsyncResult(virt_ptr<FSClient> client,
                          virt_ptr<FSCmdBlock> block,
                          FSStatus result,
                          FSErrorFlag errorMask);

void
fsClientSubmitCommand(virt_ptr<FSClientBody> clientBody,
                      virt_ptr<FSCmdBlockBody> blockBody,
                      FSFinishCmdFn finishCmdFn);

} // namespace internal

/** @} */

} // namespace cafe::coreinit

#pragma once
#include "coreinit_alarm.h"
#include "coreinit_enum.h"
#include "coreinit_ios.h"
#include "coreinit_fastmutex.h"
#include "coreinit_fs.h"
#include "coreinit_fs_cmdblock.h"
#include "coreinit_fs_cmdqueue.h"
#include "coreinit_fs_statemachine.h"

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

struct FSClient;
struct FSClientBody;
struct FSClientBodyLink;
struct FSCmdBlockBody;


/**
 * Attach parameters passed to FSAddClientEx.
 */
struct FSAttachParams
{
   be_ptr<void> userCallback;
   be_ptr<void> userContext;
};
CHECK_OFFSET(FSAttachParams, 0x00, userCallback);
CHECK_OFFSET(FSAttachParams, 0x04, userContext);
CHECK_SIZE(FSAttachParams, 0x8);


/**
 * Client container, use internal::getClientBody to get the actual data.
 */
struct FSClient
{
   uint8_t data[0x1700];
};
CHECK_SIZE(FSClient, 0x1700);


/**
 * Link entry used for FSClientBodyQueue.
 */
struct FSClientBodyLink
{
   be_ptr<FSClientBody> next;
   be_ptr<FSClientBody> prev;
};
CHECK_OFFSET(FSClientBodyLink, 0x00, next);
CHECK_OFFSET(FSClientBodyLink, 0x04, prev);
CHECK_SIZE(FSClientBodyLink, 0x8);


/**
 * A queue structure to contain FSClientBody.
 */
struct FSClientBodyQueue
{
   be_ptr<FSClientBody> head;
   be_ptr<FSClientBody> tail;
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
   be_val<IOSHandle> clientHandle;

   //! State machine.
   FSFsm fsm;

   //! Command queue of FS commands.
   FSCmdQueue cmdQueue;

   //! The last dequeued command.
   be_ptr<FSCmdBlockBody> lastDequeuedCommand;

   //! Emulated error, set with FSSetEmulatedError.
   be_val<FSAStatus> emulatedError;

   be_val<uint32_t> unk0x14CC;
   be_val<uint32_t> unk0x14D0;
   UNKNOWN(0x1560 - 0x14D4);

   //! Mutex used to protect FSClientBody data.
   OSFastMutex mutex;

   UNKNOWN(4);

   //! Alarm used by fsm for unknown reasons.
   OSAlarm fsmAlarm;

   //! Error of last FS command.
   be_val<FSAStatus> lastError;

   be_val<BOOL> isLastErrorWithoutVolume;

   //! Message used to send FsCmdHandler message when FSA async callback is received.
   FSMessage fsCmdHandlerMsg;

   //! Device name of the last mount source returned by FSGetMountSourceNext.
   char lastMountSourceDevice[0x10];

   //! Mount source type to find with FSGetMountSourceNext.
   be_val<FSMountSourceType> findMountSourceType;

   //! Link used for linked list of clients.
   FSClientBodyLink link;

   //! Pointer to unaligned FSClient structure.
   be_ptr<FSClient> client;
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
FSAddClientEx(FSClient *client,
              FSAttachParams *attachParams,
              FSErrorFlag errorMask);

FSStatus
FSAddClient(FSClient *client,
            FSErrorFlag errorMask);

FSStatus
FSDelClient(FSClient *client,
            FSErrorFlag errorMask);

FSCmdBlock *
FSGetCurrentCmdBlock(FSClient *client);

FSAStatus
FSGetEmulatedError(FSClient *client);

FSStatus
FSSetEmulatedError(FSClient *client,
                   FSAStatus error);

int32_t
FSGetErrorCodeForViewer(FSClient *client,
                        FSCmdBlock *block);

int32_t
FSGetLastErrorCodeForViewer(FSClient *client);

FSAStatus
FSGetLastError(FSClient *client);

FSVolumeState
FSGetVolumeState(FSClient *client);

namespace internal
{

FSClientBody *
fsClientGetBody(FSClient *client);

void
fsClientHandleFatalError(FSClientBody *clientBody,
                         FSAStatus error);

FSStatus
fsClientHandleShimPrepareError(FSClientBody *clientBody,
                               FSAStatus error);

FSStatus
fsClientHandleAsyncResult(FSClient *client,
                          FSCmdBlock *block,
                          FSStatus result,
                          FSErrorFlag errorMask);

void
fsClientSubmitCommand(FSClientBody *clientBody,
                      FSCmdBlockBody *blockBody,
                      FSFinishCmdFn finishCmdFn);

} // namespace internal

/** @} */

} // namespace coreinit

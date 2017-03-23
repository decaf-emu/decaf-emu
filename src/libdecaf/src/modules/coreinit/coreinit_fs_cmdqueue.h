#pragma once
#include "coreinit_enum.h"
#include "coreinit_fastmutex.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/be_ptr.h>
#include <common/cbool.h>
#include <common/structsize.h>

namespace coreinit
{

#pragma pack(push, 1)

/**
 * \ingroup coreinit_fs
 * @{
 */

struct FSCmdBlockBody;

using FSCmdQueueHandlerFn = wfunc_ptr<BOOL, FSCmdBlockBody *>;

struct FSCmdQueue
{
   //! Head of the queue.
   be_ptr<FSCmdBlockBody> head;

   //! Tail of the queue.
   be_ptr<FSCmdBlockBody> tail;

   //! Mutex used to protect queue data.
   OSFastMutex mutex;

   //! Function to call when a command is dequeued.
   FSCmdQueueHandlerFn::be dequeueCmdHandler;

   //! Number of active commands.
   be_val<uint32_t> activeCmds;

   //! Max allowed active commands (should always be 1).
   be_val<uint32_t> maxActiveCmds;

   //! Status of the command queue.
   be_val<FSCmdQueueStatus> status;
};
CHECK_OFFSET(FSCmdQueue, 0x0, head);
CHECK_OFFSET(FSCmdQueue, 0x4, tail);
CHECK_OFFSET(FSCmdQueue, 0x8, mutex);
CHECK_OFFSET(FSCmdQueue, 0x34, dequeueCmdHandler);
CHECK_OFFSET(FSCmdQueue, 0x38, activeCmds);
CHECK_OFFSET(FSCmdQueue, 0x3C, maxActiveCmds);
CHECK_OFFSET(FSCmdQueue, 0x40, status);
CHECK_SIZE(FSCmdQueue, 0x44);

#pragma pack(pop)

namespace internal
{

bool
fsCmdQueueCreate(FSCmdQueue *queue,
                 FSCmdQueueHandlerFn dequeueCmdHandler,
                 uint32_t maxActiveCmds);

void
fsCmdQueueDestroy(FSCmdQueue *queue);

void
fsCmdQueueCancelAll(FSCmdQueue *queue);

void
fsCmdQueueSuspend(FSCmdQueue *queue);

void
fsCmdQueueResume(FSCmdQueue *queue);

void
fsCmdQueueEnqueue(FSCmdQueue *queue,
                  FSCmdBlockBody *blockBody,
                  bool sortLE);

void
fsCmdQueuePushFront(FSCmdQueue *queue,
                    FSCmdBlockBody *blockBody);

FSCmdBlockBody *
fsCmdQueuePopFront(FSCmdQueue *queue);

bool
fsCmdQueueBeginCmd(FSCmdQueue *queue);

void
fsCmdQueueFinishCmd(FSCmdQueue *queue);

bool
fsCmdQueueProcessCmd(FSCmdQueue *queue);

} // namespace internal

/** @} */

} // namespace coreinit

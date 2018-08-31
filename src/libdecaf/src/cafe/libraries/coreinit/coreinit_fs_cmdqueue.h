#pragma once
#include "coreinit_enum.h"
#include "coreinit_fastmutex.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

#pragma pack(push, 1)

/**
 * \ingroup coreinit_fs
 * @{
 */

struct FSCmdBlockBody;

using FSCmdQueueHandlerFn = virt_func_ptr<BOOL(virt_ptr<FSCmdBlockBody>)>;

struct FSCmdQueue
{
   //! Head of the queue.
   be2_virt_ptr<FSCmdBlockBody> head;

   //! Tail of the queue.
   be2_virt_ptr<FSCmdBlockBody> tail;

   //! Mutex used to protect queue data.
   be2_struct<OSFastMutex> mutex;

   //! Function to call when a command is dequeued.
   be2_val<FSCmdQueueHandlerFn> dequeueCmdHandler;

   //! Number of active commands.
   be2_val<uint32_t> activeCmds;

   //! Max allowed active commands (should always be 1).
   be2_val<uint32_t> maxActiveCmds;

   //! Status of the command queue.
   be2_val<FSCmdQueueStatus> status;
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
fsCmdQueueCreate(virt_ptr<FSCmdQueue> queue,
                 FSCmdQueueHandlerFn dequeueCmdHandler,
                 uint32_t maxActiveCmds);

void
fsCmdQueueDestroy(virt_ptr<FSCmdQueue> queue);

void
fsCmdQueueCancelAll(virt_ptr<FSCmdQueue> queue);

void
fsCmdQueueSuspend(virt_ptr<FSCmdQueue> queue);

void
fsCmdQueueResume(virt_ptr<FSCmdQueue> queue);

void
fsCmdQueueEnqueue(virt_ptr<FSCmdQueue> queue,
                  virt_ptr<FSCmdBlockBody> blockBody,
                  bool sortLE);

void
fsCmdQueuePushFront(virt_ptr<FSCmdQueue> queue,
                    virt_ptr<FSCmdBlockBody> blockBody);

virt_ptr<FSCmdBlockBody>
fsCmdQueuePopFront(virt_ptr<FSCmdQueue> queue);

bool
fsCmdQueueBeginCmd(virt_ptr<FSCmdQueue> queue);

void
fsCmdQueueFinishCmd(virt_ptr<FSCmdQueue> queue);

bool
fsCmdQueueProcessCmd(virt_ptr<FSCmdQueue> queue);

} // namespace internal

/** @} */

} // namespace cafe::coreinit

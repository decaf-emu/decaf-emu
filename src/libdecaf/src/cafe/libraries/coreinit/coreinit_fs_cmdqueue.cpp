#include "coreinit_fastmutex.h"
#include "coreinit_fs_client.h"
#include "coreinit_fs_cmdblock.h"
#include "coreinit_fs_cmdqueue.h"
#include "coreinit_internal_queue.h"
#include "cafe/cafe_ppc_interface_invoke.h"
#include <libcpu/cpu.h>

namespace cafe::coreinit
{

namespace internal
{

struct FSCmdSortFuncLT
{
   bool operator ()(virt_ptr<FSCmdBlockBody> lhs,
                    virt_ptr<FSCmdBlockBody> rhs) const
   {
      return lhs->priority < rhs->priority;
   }
};

struct FSCmdSortFuncLE
{
   bool operator ()(virt_ptr<FSCmdBlockBody> lhs,
                    virt_ptr<FSCmdBlockBody> rhs) const
   {
      return lhs->priority <= rhs->priority;
   }
};

using CmdQueueLT = internal::SortedQueue<FSCmdQueue, FSCmdBlockBodyLink,
                                         FSCmdBlockBody, &FSCmdBlockBody::link,
                                         FSCmdSortFuncLT>;

using CmdQueueLE = internal::SortedQueue<FSCmdQueue, FSCmdBlockBodyLink,
                                         FSCmdBlockBody, &FSCmdBlockBody::link,
                                          FSCmdSortFuncLE>;


/**
 * Initialise an FSCmdQueue structure.
 */
bool
fsCmdQueueCreate(virt_ptr<FSCmdQueue> queue,
                 FSCmdQueueHandlerFn dequeueCmdHandler,
                 uint32_t maxActiveCmds)
{
   if (!queue || !dequeueCmdHandler) {
      return false;
   }

   queue->head = nullptr;
   queue->tail = nullptr;
   queue->dequeueCmdHandler = dequeueCmdHandler;
   queue->activeCmds = 0u;
   queue->maxActiveCmds = maxActiveCmds;
   OSFastMutex_Init(virt_addrof(queue->mutex), nullptr);
   return true;
}


/**
 * Destroy a FSCmdQueue structure.
 */
void
fsCmdQueueDestroy(virt_ptr<FSCmdQueue> queue)
{
   fsCmdQueueCancelAll(queue);
}


/**
 * Cancels all commands in a FSCmdQueue structure.
 */
void
fsCmdQueueCancelAll(virt_ptr<FSCmdQueue> queue)
{
   OSFastMutex_Lock(virt_addrof(queue->mutex));

   while (auto cmd = fsCmdQueuePopFront(queue)) {
      cmd->status = FSCmdBlockStatus::Cancelled;
   }

   OSFastMutex_Unlock(virt_addrof(queue->mutex));
}


/**
 * Prevent the FSCmdQueue from dequeuing new commands.
 *
 * Sets the FSCmdQueueStatus::Suspended flag.
 */
void
fsCmdQueueSuspend(virt_ptr<FSCmdQueue> queue)
{
   queue->status |= FSCmdQueueStatus::Suspended;
}


/**
 * Allow the FSCmdQueue to dequeuing new commands.
 *
 * Clears the FSCmdQueueStatus::Suspended flag.
 */
void
fsCmdQueueResume(virt_ptr<FSCmdQueue> queue)
{
   queue->status &= ~FSCmdQueueStatus::Suspended;
}


/**
 * Insert a command into the queue, sorted by priority.
 */
void
fsCmdQueueEnqueue(virt_ptr<FSCmdQueue> queue,
                  virt_ptr<FSCmdBlockBody> blockBody,
                  bool sortLE)
{
   if (sortLE) {
      CmdQueueLE::insert(queue, blockBody);
   } else {
      CmdQueueLT::insert(queue, blockBody);
   }
}


/**
 * Insert a command at the front of the queue.
 */
void
fsCmdQueuePushFront(virt_ptr<FSCmdQueue> queue,
                    virt_ptr<FSCmdBlockBody> blockBody)
{
   blockBody->link.prev = nullptr;
   blockBody->link.next = queue->head;

   if (blockBody->link.next) {
      blockBody->link.next->link.prev = blockBody;
   }

   queue->head = blockBody;
}


/**
 * Pop a command from the front of the queue.
 */
virt_ptr<FSCmdBlockBody>
fsCmdQueuePopFront(virt_ptr<FSCmdQueue> queue)
{
   return CmdQueueLT::popFront(queue);
}


/**
 * Begin a command.
 *
 * Increases the active command count.
 *
 * \retval true
 * Returns true if a command can begin.
 *
 * \retval false
 * Returns false if there is already the max amount of active commands.
 */
bool
fsCmdQueueBeginCmd(virt_ptr<FSCmdQueue> queue)
{
   if (queue->status & FSCmdQueueStatus::MaxActiveCommands ||
       queue->status & FSCmdQueueStatus::Suspended) {
      return false;
   }

   queue->activeCmds += 1;

   if (queue->activeCmds >= queue->maxActiveCmds) {
      queue->status |= FSCmdQueueStatus::MaxActiveCommands;
   }

   return true;
}


/**
 * Finish a command.
 *
 * Decreases the active command count.
 */
void
fsCmdQueueFinishCmd(virt_ptr<FSCmdQueue> queue)
{
   if (queue->activeCmds) {
      queue->activeCmds -= 1;
   }

   if (queue->activeCmds < queue->maxActiveCmds) {
      queue->status &= ~FSCmdQueueStatus::MaxActiveCommands;
   }
}


/**
 * Process a command from the queue.
 *
 * Pops a command from the front of the queue and calls the dequeued command
 * handler on it.
 */
bool
fsCmdQueueProcessCmd(virt_ptr<FSCmdQueue> queue)
{
   OSFastMutex_Lock(virt_addrof(queue->mutex));
   auto activeCmd = fsCmdQueueBeginCmd(queue);
   OSFastMutex_Unlock(virt_addrof(queue->mutex));

   if (!activeCmd) {
      return false;
   }

   OSFastMutex_Lock(virt_addrof(queue->mutex));
   auto cmd = fsCmdQueuePopFront(queue);

   if (cmd) {
      auto clientBody = cmd->clientBody;
      clientBody->lastDequeuedCommand = cmd;
      cmd->status = FSCmdBlockStatus::DeqeuedCommand;
      OSFastMutex_Unlock(virt_addrof(queue->mutex));

      // Call the dequeue command handler
      if (cafe::invoke(cpu::this_core::state(),
                       queue->dequeueCmdHandler,
                       cmd)) {
         return true;
      }

      OSFastMutex_Lock(virt_addrof(queue->mutex));
   }

   fsCmdQueueFinishCmd(queue);
   OSFastMutex_Unlock(virt_addrof(queue->mutex));
   return true;
}

} // namespace internal

} // namespace cafe::coreinit

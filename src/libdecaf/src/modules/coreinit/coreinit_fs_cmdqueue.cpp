#include "coreinit_fastmutex.h"
#include "coreinit_fs_client.h"
#include "coreinit_fs_cmdblock.h"
#include "coreinit_fs_cmdqueue.h"
#include "coreinit_internal_queue.h"
#include "ppcutils/wfunc_call.h"

namespace coreinit
{

namespace internal
{

bool
fsCmdSortFuncLT(FSCmdBlockBody *lhs,
                FSCmdBlockBody *rhs)
{
   return lhs->priority < rhs->priority;
}

bool
fsCmdSortFuncLE(FSCmdBlockBody *lhs,
                FSCmdBlockBody *rhs)
{
   return lhs->priority <= rhs->priority;
}

using CmdQueueLT = internal::SortedQueue<FSCmdQueue, FSCmdBlockBodyLink,
                                         FSCmdBlockBody, &FSCmdBlockBody::link,
                                         fsCmdSortFuncLT>;

using CmdQueueLE = internal::SortedQueue<FSCmdQueue, FSCmdBlockBodyLink,
                                         FSCmdBlockBody, &FSCmdBlockBody::link,
                                         fsCmdSortFuncLE>;


/**
 * Initialise an FSCmdQueue structure.
 */
bool
fsCmdQueueCreate(FSCmdQueue *queue,
                 FSCmdQueueHandlerFn dequeueCmdHandler,
                 uint32_t maxActiveCmds)
{
   if (!queue || !dequeueCmdHandler) {
      return false;
   }

   queue->head = nullptr;
   queue->tail = nullptr;
   queue->dequeueCmdHandler = dequeueCmdHandler;
   queue->activeCmds = 0;
   queue->maxActiveCmds = maxActiveCmds;
   OSFastMutex_Init(&queue->mutex, nullptr);
   return true;
}


/**
 * Destroy a FSCmdQueue structure.
 */
void
fsCmdQueueDestroy(FSCmdQueue *queue)
{
   fsCmdQueueCancelAll(queue);
}


/**
 * Cancels all commands in a FSCmdQueue structure.
 */
void
fsCmdQueueCancelAll(FSCmdQueue *queue)
{
   OSFastMutex_Lock(&queue->mutex);

   while (auto cmd = fsCmdQueuePopFront(queue)) {
      cmd->status = FSCmdBlockStatus::Cancelled;
   }

   OSFastMutex_Unlock(&queue->mutex);
}


/**
 * Prevent the FSCmdQueue from dequeuing new commands.
 *
 * Sets the FSCmdQueueStatus::Suspended flag.
 */
void
fsCmdQueueSuspend(FSCmdQueue *queue)
{
   queue->status |= FSCmdQueueStatus::Suspended;
}


/**
 * Allow the FSCmdQueue to dequeuing new commands.
 *
 * Clears the FSCmdQueueStatus::Suspended flag.
 */
void
fsCmdQueueResume(FSCmdQueue *queue)
{
   queue->status &= ~FSCmdQueueStatus::Suspended;
}


/**
 * Insert a command into the queue, sorted by priority.
 */
void
fsCmdQueueEnqueue(FSCmdQueue *queue,
                  FSCmdBlockBody *blockBody,
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
fsCmdQueuePushFront(FSCmdQueue *queue,
                    FSCmdBlockBody *blockBody)
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
FSCmdBlockBody *
fsCmdQueuePopFront(FSCmdQueue *queue)
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
fsCmdQueueBeginCmd(FSCmdQueue *queue)
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
fsCmdQueueFinishCmd(FSCmdQueue *queue)
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
fsCmdQueueProcessCmd(FSCmdQueue *queue)
{
   OSFastMutex_Lock(&queue->mutex);
   auto activeCmd = fsCmdQueueBeginCmd(queue);
   OSFastMutex_Unlock(&queue->mutex);

   if (!activeCmd) {
      return false;
   }

   OSFastMutex_Lock(&queue->mutex);
   auto cmd = fsCmdQueuePopFront(queue);

   if (cmd) {
      auto clientBody = cmd->clientBody;
      clientBody->lastDequeuedCommand = cmd;
      cmd->status = FSCmdBlockStatus::DeqeuedCommand;
      OSFastMutex_Unlock(&queue->mutex);

      // Call the dequeue command handler
      if (queue->dequeueCmdHandler(cmd)) {
         return true;
      }

      OSFastMutex_Lock(&queue->mutex);
   }

   fsCmdQueueFinishCmd(queue);
   OSFastMutex_Unlock(&queue->mutex);
   return true;
}

} // namespace internal

} // namespace coreinit

#include <algorithm>
#include "coreinit.h"
#include "coreinit_taskqueue.h"
#include "ppcutils/wfunc_call.h"

namespace coreinit
{


/**
 * Initialise a task queue structure.
 */
void
MPInitTaskQ(MPTaskQueue *queue,
            be_ptr<MPTask> *taskBuffer,
            uint32_t taskBufferLen)
{
   OSInitSpinLock(&queue->lock);
   queue->self = queue;
   queue->state = MPTaskQueueState::Initialised;
   queue->tasks = 0;
   queue->tasksReady = 0;
   queue->tasksRunning = 0;
   queue->tasksFinished = 0;
   queue->queueIndex = 0;
   queue->queueSize = 0;
   queue->queue = taskBuffer;
   queue->queueMaxSize = taskBufferLen;
}


/**
 * Terminates a task queue.
 *
 * Yes this really does only return TRUE in coreinit.rpl
 */
BOOL
MPTermTaskQ(MPTaskQueue *queue)
{
   return TRUE;
}


/**
 * Get the status of a task queue.
 */
BOOL
MPGetTaskQInfo(MPTaskQueue *queue,
               MPTaskQueueInfo *info)
{
   OSUninterruptibleSpinLock_Acquire(&queue->lock);
   info->state = queue->state;
   info->tasks = queue->tasks;
   info->tasksReady = queue->tasksReady;
   info->tasksRunning = queue->tasksRunning;
   info->tasksFinished = queue->tasksFinished;
   OSUninterruptibleSpinLock_Release(&queue->lock);
   return TRUE;
}


/**
 * Starts a task queue.
 *
 * Sets the task state to Ready.
 *
 * \return Returns true if state was previously Initialised or Stopped.
 */
BOOL
MPStartTaskQ(MPTaskQueue *queue)
{
   OSUninterruptibleSpinLock_Acquire(&queue->lock);

   if (queue->state == MPTaskQueueState::Initialised || queue->state == MPTaskQueueState::Stopped) {
      queue->state = MPTaskQueueState::Ready;
      OSUninterruptibleSpinLock_Release(&queue->lock);
      return TRUE;
   }

   OSUninterruptibleSpinLock_Release(&queue->lock);
   return FALSE;
}


/**
 * Stops a task queue.
 *
 * If there are tasks running the state is set to  Stopping.
 * If there are no tasks running the state is set to Stopped.
 *
 * \return Returns FALSE if the task queue is not in the Ready state.
 */
BOOL
MPStopTaskQ(MPTaskQueue *queue)
{
   OSUninterruptibleSpinLock_Acquire(&queue->lock);

   if (queue->state != MPTaskQueueState::Ready) {
      OSUninterruptibleSpinLock_Release(&queue->lock);
      return FALSE;
   }

   if (queue->tasksRunning == 0) {
      queue->state = MPTaskQueueState::Stopped;
   } else {
      queue->state = MPTaskQueueState::Stopping;
   }

   OSUninterruptibleSpinLock_Release(&queue->lock);
   return TRUE;
}


/**
 * Resets the state of the task queue.
 *
 * This does not remove any tasks from the queue. It just resets the task queue
 * such that all tasks are ready to be run again.
 */
BOOL
MPResetTaskQ(MPTaskQueue *queue)
{
   OSUninterruptibleSpinLock_Acquire(&queue->lock);

   if (queue->state != MPTaskQueueState::Finished &&
       queue->state != MPTaskQueueState::Stopped) {
      OSUninterruptibleSpinLock_Release(&queue->lock);
      return FALSE;
   }

   queue->state = MPTaskQueueState::Initialised;
   queue->tasks = queue->queueSize;
   queue->tasksReady = queue->queueSize;
   queue->tasksRunning = 0;
   queue->tasksFinished = 0;
   queue->queueIndex = 0;

   for (auto i = 0u; i < queue->tasks; ++i) {
      auto task = queue->queue[i];
      task->result = 0;
      task->coreID = CoreCount;
      task->duration = 0;
      task->state = MPTaskState::Ready;
   }

   OSUninterruptibleSpinLock_Release(&queue->lock);
   return TRUE;
}


/**
 * Add a task to the end of the queue.
 *
 * Returns FALSE if the task state is not set to Initialised.
 * Returns FALSE if the task queue is full.
 * Returns FALSE if the task queue state is not valid.
 *
 * \return Returns TRUE if the task was added to the queue.
 */
BOOL
MPEnqueTask(MPTaskQueue *queue,
            MPTask *task)
{
   if (task->state != MPTaskState::Initialised) {
      return FALSE;
   }

   OSUninterruptibleSpinLock_Acquire(&queue->lock);

   if (queue->queueSize >= queue->queueMaxSize) {
      OSUninterruptibleSpinLock_Release(&queue->lock);
      return FALSE;
   }

   if (queue->state < MPTaskQueueState::Initialised || queue->state > MPTaskQueueState::Finished) {
      OSUninterruptibleSpinLock_Release(&queue->lock);
      return FALSE;
   }

   task->queue = queue;
   task->state = MPTaskState::Ready;

   queue->tasks++;
   queue->tasksReady++;
   queue->queue[queue->queueSize] = task;
   queue->queueSize++;

   if (queue->state == MPTaskQueueState::Finished) {
      queue->state = MPTaskQueueState::Ready;
   }

   OSUninterruptibleSpinLock_Release(&queue->lock);
   return TRUE;
}


/**
 * Dequeue 1 task at queueIndex
 *
 * Does not remove tasks from queue buffer.
 *
 * \return Returns dequeued task.
 */
MPTask *
MPDequeTask(MPTaskQueue *queue)
{
   OSUninterruptibleSpinLock_Acquire(&queue->lock);

   if (queue->state != MPTaskQueueState::Ready
    || queue->queueIndex == queue->queueSize) {
      OSUninterruptibleSpinLock_Release(&queue->lock);
      return nullptr;
   }

   auto task = queue->queue[queue->queueIndex];
   queue->queueIndex++;
   OSUninterruptibleSpinLock_Release(&queue->lock);
   return task;
}


/**
 * Dequeue N tasks from queueIndex
 *
 * Does not remove tasks from queue buffer.
 *
 * \return Returns number of tasks dequeued.
 */
uint32_t
MPDequeTasks(MPTaskQueue *queue,
             be_ptr<MPTask> *taskBuffer,
             uint32_t taskBufferLen)
{
   OSUninterruptibleSpinLock_Acquire(&queue->lock);
   uint32_t count, available;

   if (queue->state != MPTaskQueueState::Ready) {
      OSUninterruptibleSpinLock_Release(&queue->lock);
      return 0;
   }

   available = queue->queueSize - queue->queueIndex;
   count = std::min(available, taskBufferLen);

   for (auto i = 0u; i < count; ++i) {
      taskBuffer[i] = queue->queue[queue->queueIndex];
      queue->queueIndex++;
   }

   OSUninterruptibleSpinLock_Release(&queue->lock);
   return count;
}


/**
 * Busy wait until state matches mask.
 *
 * \return Always returns TRUE.
 */
BOOL
MPWaitTaskQ(MPTaskQueue *queue,
            MPTaskQueueState mask)
{
   while ((queue->state & mask) == 0);
   return TRUE;
}


/**
 * Busy wait with timeout until state matches mask.
 *
 * \return Returns FALSE if wait timed out.
 */
BOOL
MPWaitTaskQWithTimeout(MPTaskQueue *queue,
                       MPTaskQueueState mask,
                       OSTime timeoutNS)
{
   auto start = OSGetTime();
   auto end = start + internal::nanosToTicks(timeoutNS);

   while ((queue->state & mask) == 0) {
      if (OSGetTime() >= end) {
         break;
      }
   }

   return (queue->state & mask) != 0;
}


/**
 * Print debug information about task queue.
 */
BOOL
MPPrintTaskQStats(MPTaskQueue *queue,
                  uint32_t unk)
{
   // TODO: Implement MPPrintTaskQStats
   return TRUE;
}


/**
 * Initialises a task structure.
 */
void
MPInitTask(MPTask *task,
           MPTaskFunc func,
           uint32_t userArg1,
           uint32_t userArg2)
{
   task->self = task;
   task->queue = nullptr;
   task->state = MPTaskState::Initialised;
   task->func = func;
   task->userArg1 = userArg1;
   task->userArg2 = userArg2;
   task->result = 0;
   task->coreID = CoreCount;
   task->duration = 0;
   task->userData = nullptr;
}


/**
 * Terminates a task.
 *
 * Yes this really does only return TRUE in coreinit.rpl
 */
BOOL
MPTermTask(MPTask *task)
{
   return TRUE;
}


/**
 * Get information about a task.
 *
 * \return Returns TRUE if successful.
 */
BOOL
MPGetTaskInfo(MPTask *task,
              MPTaskInfo *info)
{
   info->coreID = task->coreID;
   info->duration = task->duration;
   info->result = task->result;
   info->state = task->state;
   return TRUE;
}


/**
 * Returns a task's user data which can be set with MPSetTaskUserData.
 */
void *
MPGetTaskUserData(MPTask *task)
{
   return task->userData;
}


/**
 * Sets a task's user data which can be retrieved with MPGetTaskUserData.
 */
void
MPSetTaskUserData(MPTask *task, void *userData)
{
   task->userData = userData;
}


/**
 * Run N tasks from queue.
 *
 * \param tasks Number of tasks to dequeue and run at once
 *
 * Does not remove tasks from queue.
 * Can be run from multiple threads at once.
 *
 * Side Effects:
 * - Sets state to Stopped if state is Stopping and tasksRunning reaches 0.
 * - Sets state to Finished if all tasks are finished.
 * - TasksReady -> TasksRunning -> TasksFinished.
 *
 * Returns TRUE if at least 1 task is run.
 */
BOOL
MPRunTasksFromTaskQ(MPTaskQueue *queue,
                    uint32_t tasks)
{
   BOOL result = FALSE;

   while (queue->state == MPTaskQueueState::Ready) {
      uint32_t first, count, available;

      OSUninterruptibleSpinLock_Acquire(&queue->lock);
      available = queue->queueSize - queue->queueIndex;
      count = std::min(available, tasks);
      first = queue->queueIndex;

      queue->tasksReady -= count;
      queue->tasksRunning += count;
      queue->queueIndex += count;
      OSUninterruptibleSpinLock_Release(&queue->lock);

      if (count == 0) {
         // Nothing to run, lets go home!
         break;
      }

      // Result is TRUE if at least 1 task is run
      result = TRUE;

      // Mark all tasks as running
      for (auto i = 0u; i < count; ++i) {
         auto task = queue->queue[first + i];
         task->state = MPTaskState::Running;
         task->coreID = OSGetCoreId();
      }

      // Run all tasks
      for (auto i = 0u; i < count; ++i) {
         auto task = queue->queue[first + i];
         auto start = OSGetTime();
         task->result = task->func(task->userArg1, task->userArg2);
         task->state = MPTaskState::Finished;
         task->duration = OSGetTime() - start;
      }

      OSUninterruptibleSpinLock_Acquire(&queue->lock);
      queue->tasksRunning -= count;
      queue->tasksFinished += count;

      if (queue->state == MPTaskQueueState::Stopping && queue->tasksRunning == 0) {
         queue->state = MPTaskQueueState::Stopped;
      }

      if (queue->tasks == queue->tasksFinished) {
         queue->state = MPTaskQueueState::Finished;
      }

      OSUninterruptibleSpinLock_Release(&queue->lock);
   }

   return result;
}


/**
 * Run a specific task.
 *
 * The task must belong to a queue.
 * The task must be in the Ready state.
 *
 * \return Returns TRUE if task was run.
 */
BOOL
MPRunTask(MPTask *task)
{
   auto queue = task->queue;

   if (task->state != MPTaskState::Ready) {
      return FALSE;
   }

   if (!queue
     || queue->state == MPTaskQueueState::Stopping
     || queue->state == MPTaskQueueState::Stopped) {
      return FALSE;
   }

   OSUninterruptibleSpinLock_Acquire(&queue->lock);
   queue->tasksReady--;
   queue->tasksRunning++;
   OSUninterruptibleSpinLock_Release(&queue->lock);

   task->state = MPTaskState::Running;
   task->coreID = OSGetCoreId();

   auto start = OSGetTime();
   task->result = task->func(task->userArg1, task->userArg2);
   task->duration = OSGetTime() - start;

   task->state = MPTaskState::Finished;

   OSUninterruptibleSpinLock_Acquire(&queue->lock);
   queue->tasksRunning--;
   queue->tasksFinished++;

   if (queue->state == MPTaskQueueState::Stopping && queue->tasksRunning == 0) {
      queue->state = MPTaskQueueState::Stopped;
   }

   if (queue->tasks == queue->tasksFinished) {
      queue->state = MPTaskQueueState::Finished;
   }

   OSUninterruptibleSpinLock_Release(&queue->lock);
   return TRUE;
}

void
Module::registerTaskQueueFunctions()
{
   RegisterKernelFunction(MPInitTaskQ);
   RegisterKernelFunction(MPTermTaskQ);
   RegisterKernelFunction(MPGetTaskQInfo);
   RegisterKernelFunction(MPStartTaskQ);
   RegisterKernelFunction(MPStopTaskQ);
   RegisterKernelFunction(MPResetTaskQ);
   RegisterKernelFunction(MPEnqueTask);
   RegisterKernelFunction(MPDequeTask);
   RegisterKernelFunction(MPDequeTasks);
   RegisterKernelFunction(MPWaitTaskQ);
   RegisterKernelFunction(MPWaitTaskQWithTimeout);
   RegisterKernelFunction(MPPrintTaskQStats);
   RegisterKernelFunction(MPInitTask);
   RegisterKernelFunction(MPTermTask);
   RegisterKernelFunction(MPGetTaskInfo);
   RegisterKernelFunction(MPGetTaskUserData);
   RegisterKernelFunction(MPSetTaskUserData);
   RegisterKernelFunction(MPRunTasksFromTaskQ);
   RegisterKernelFunction(MPRunTask);
}

} // namespace coreinit

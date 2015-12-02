#include <algorithm>
#include "coreinit.h"
#include "coreinit_taskqueue.h"
#include "utils/wfunc_call.h"

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

BOOL
MPTermTaskQ(MPTaskQueue *queue)
{
   return TRUE;
}

BOOL
MPGetTaskQInfo(MPTaskQueue *queue,
               MPTaskQueueInfo *info)
{
   ScopedSpinLock lock(&queue->lock);
   info->state = queue->state;
   info->tasks = queue->tasks;
   info->tasksReady = queue->tasksReady;
   info->tasksRunning = queue->tasksRunning;
   info->tasksFinished = queue->tasksFinished;
   return TRUE;
}

BOOL
MPStartTaskQ(MPTaskQueue *queue)
{
   ScopedSpinLock lock(&queue->lock);

   if (queue->state == MPTaskQueueState::Initialised || queue->state == MPTaskQueueState::Stopped) {
      queue->state = MPTaskQueueState::Ready;
      return TRUE;
   }

   return FALSE;
}

BOOL
MPStopTaskQ(MPTaskQueue *queue)
{
   ScopedSpinLock lock(&queue->lock);

   if (queue->state == MPTaskQueueState::Ready) {
      return FALSE;
   }

   if (queue->tasksRunning == 0) {
      queue->state = MPTaskQueueState::Stopped;
   } else {
      queue->state = MPTaskQueueState::Stopping;
   }

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
   ScopedSpinLock lock(&queue->lock);

   if (queue->state != MPTaskQueueState::Finished &&
       queue->state != MPTaskQueueState::Stopped) {
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

   return TRUE;
}


// Add a task to the end of the queue.
BOOL
MPEnqueTask(MPTaskQueue *queue,
            MPTask *task)
{
   if (task->state != MPTaskState::Initialised) {
      return FALSE;
   }

   ScopedSpinLock lock(&queue->lock);

   if (queue->queueSize >= queue->queueMaxSize) {
      return FALSE;
   }

   if (queue->state < MPTaskQueueState::Initialised || queue->state > MPTaskQueueState::Finished) {
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

   return TRUE;
}


/**
* Dequeue 1 task at queueIndex
*
* Does not remove tasks from queue buffer.
*/

MPTask *
MPDequeTask(MPTaskQueue *queue)
{
   ScopedSpinLock lock(&queue->lock);

   if (queue->state != MPTaskQueueState::Ready) {
      return nullptr;
   }

   if (queue->queueIndex == queue->queueSize) {
      return nullptr;
   }

   auto task = queue->queue[queue->queueIndex];
   queue->queueIndex++;
   return task;
}


/**
* Dequeue N tasks from queueIndex
*
* Does not remove tasks from queue buffer.
*/

uint32_t
MPDequeTasks(MPTaskQueue *queue,
             be_ptr<MPTask> *taskBuffer,
             uint32_t taskBufferLen)
{
   ScopedSpinLock lock(&queue->lock);
   uint32_t count, available;

   if (queue->state != MPTaskQueueState::Ready) {
      return 0;
   }

   available = queue->queueSize - queue->queueIndex;
   count = std::min(available, taskBufferLen);

   for (auto i = 0u; i < count; ++i) {
      taskBuffer[i] = queue->queue[queue->queueIndex];
      queue->queueIndex++;
   }

   return count;
}


// Busy wait until state mask matches mask (ewwww)
BOOL
MPWaitTaskQ(MPTaskQueue *queue, MPTaskQueueState mask)
{
   while ((queue->state & mask) == 0);
   return TRUE;
}


BOOL
MPWaitTaskQWithTimeout(MPTaskQueue *queue, MPTaskQueueState mask, OSTime timeout)
{
   auto start = OSGetTime();
   auto end = start + timeout;

   while ((queue->state & mask) == 0) {
      if (OSGetTime() >= end) {
         break;
      }
   }

   return (queue->state & mask) != 0;
}


BOOL
MPPrintTaskQStats(MPTaskQueue *queue, uint32_t unk)
{
   return TRUE;
}


void
MPInitTask(MPTask *task, MPTaskFunc func, uint32_t userArg1, uint32_t userArg2)
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


BOOL
MPTermTask(MPTask *task)
{
   return TRUE;
}


BOOL
MPGetTaskInfo(MPTask *task, MPTaskInfo *info)
{
   info->coreID = task->coreID;
   info->duration = task->duration;
   info->result = task->result;
   info->state = task->state;
   return TRUE;
}


void *
MPGetTaskUserData(MPTask *task)
{
   return task->userData;
}


void
MPSetTaskUserData(MPTask *task, void *userData)
{
   task->userData = userData;
}


/**
* Run N tasks from queue.
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
      tasks -= count;

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


// Run 1 task which belongs to a queue
BOOL
MPRunTask(MPTask *task)
{
   auto queue = task->queue;

   if (task->state != MPTaskState::Ready || !queue) {
      return FALSE;
   }

   if (queue->state == MPTaskQueueState::Stopping || queue->state == MPTaskQueueState::Stopped) {
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
CoreInit::registerTaskQueueFunctions()
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

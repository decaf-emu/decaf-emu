#pragma once
#include "types.h"
#include "coreinit_core.h"
#include "coreinit_enum.h"
#include "coreinit_spinlock.h"
#include "coreinit_time.h"
#include "utils/be_val.h"
#include "utils/be_volatile.h"
#include "utils/structsize.h"
#include "utils/virtual_ptr.h"
#include "utils/wfunc_ptr.h"

namespace coreinit
{

/**
 * \defgroup coreinit_taskqueue Task Queue
 * \ingroup coreinit
 * @{
 */

struct MPTaskQueue;

#pragma pack(push, 1)

using MPTaskFunc = wfunc_ptr<uint32_t, uint32_t, uint32_t>;
using be_MPTaskFunc = be_wfunc_ptr<uint32_t, uint32_t, uint32_t>;

struct MPTaskInfo
{
   be_val<MPTaskState> state;
   be_val<uint32_t> result;
   be_val<uint32_t> coreID;
   be_val<OSTime> duration;
};
CHECK_OFFSET(MPTaskInfo, 0x00, state);
CHECK_OFFSET(MPTaskInfo, 0x04, result);
CHECK_OFFSET(MPTaskInfo, 0x08, coreID);
CHECK_OFFSET(MPTaskInfo, 0x0C, duration);
CHECK_SIZE(MPTaskInfo, 0x14);

struct MPTask
{
   be_ptr<MPTask> self;
   be_ptr<MPTaskQueue> queue;
   be_volatile<MPTaskState> state;
   be_MPTaskFunc func;
   be_val<uint32_t> userArg1;
   be_val<uint32_t> userArg2;
   be_val<uint32_t> result;
   be_val<uint32_t> coreID;
   be_val<OSTime> duration;
   be_ptr<void> userData;
};
CHECK_OFFSET(MPTask, 0x00, self);
CHECK_OFFSET(MPTask, 0x04, queue);
CHECK_OFFSET(MPTask, 0x08, state);
CHECK_OFFSET(MPTask, 0x0C, func);
CHECK_OFFSET(MPTask, 0x10, userArg1);
CHECK_OFFSET(MPTask, 0x14, userArg2);
CHECK_OFFSET(MPTask, 0x18, result);
CHECK_OFFSET(MPTask, 0x1C, coreID);
CHECK_OFFSET(MPTask, 0x20, duration);
CHECK_OFFSET(MPTask, 0x28, userData);
CHECK_SIZE(MPTask, 0x2C);

struct MPTaskQueueInfo
{
   be_val<MPTaskQueueState> state;
   be_val<uint32_t> tasks;
   be_val<uint32_t> tasksReady;
   be_val<uint32_t> tasksRunning;
   be_val<uint32_t> tasksFinished;
};
CHECK_OFFSET(MPTaskQueueInfo, 0x00, state);
CHECK_OFFSET(MPTaskQueueInfo, 0x04, tasks);
CHECK_OFFSET(MPTaskQueueInfo, 0x08, tasksReady);
CHECK_OFFSET(MPTaskQueueInfo, 0x0C, tasksRunning);
CHECK_OFFSET(MPTaskQueueInfo, 0x10, tasksFinished);
CHECK_SIZE(MPTaskQueueInfo, 0x14);

struct MPTaskQueue
{
   be_ptr<MPTaskQueue> self;
   be_volatile<MPTaskQueueState> state;
   be_val<uint32_t> tasks;
   be_val<uint32_t> tasksReady;
   be_val<uint32_t> tasksRunning;
   UNKNOWN(4);
   be_val<uint32_t> tasksFinished;
   UNKNOWN(8);
   be_val<uint32_t> queueIndex;
   UNKNOWN(8);
   be_val<uint32_t> queueSize;
   UNKNOWN(4);
   be_ptr<be_ptr<MPTask>> queue;
   be_val<uint32_t> queueMaxSize;
   OSSpinLock lock;
};
CHECK_OFFSET(MPTaskQueue, 0x00, self);
CHECK_OFFSET(MPTaskQueue, 0x04, state);
CHECK_OFFSET(MPTaskQueue, 0x08, tasks);
CHECK_OFFSET(MPTaskQueue, 0x0C, tasksReady);
CHECK_OFFSET(MPTaskQueue, 0x10, tasksRunning);
CHECK_OFFSET(MPTaskQueue, 0x18, tasksFinished);
CHECK_OFFSET(MPTaskQueue, 0x24, queueIndex);
CHECK_OFFSET(MPTaskQueue, 0x30, queueSize);
CHECK_OFFSET(MPTaskQueue, 0x38, queue);
CHECK_OFFSET(MPTaskQueue, 0x3C, queueMaxSize);
CHECK_OFFSET(MPTaskQueue, 0x40, lock);
CHECK_SIZE(MPTaskQueue, 0x50);

#pragma pack(pop)

void
MPInitTaskQ(MPTaskQueue *queue,
            be_ptr<MPTask> *taskBuffer,
            uint32_t taskBufferLen);

BOOL
MPTermTaskQ(MPTaskQueue *queue);

BOOL
MPGetTaskQInfo(MPTaskQueue *queue,
               MPTaskQueueInfo *info);

BOOL
MPStartTaskQ(MPTaskQueue *queue);

BOOL
MPStopTaskQ(MPTaskQueue *queue);

BOOL
MPResetTaskQ(MPTaskQueue *queue);

BOOL
MPEnqueTask(MPTaskQueue *queue,
            MPTask *task);

MPTask *
MPDequeTask(MPTaskQueue *queue);

uint32_t
MPDequeTasks(MPTaskQueue *queue,
             be_ptr<MPTask> *taskBuffer,
             uint32_t taskBufferLen);

BOOL
MPWaitTaskQ(MPTaskQueue *queue,
            MPTaskQueueState mask);

BOOL
MPWaitTaskQWithTimeout(MPTaskQueue *queue,
                       MPTaskQueueState wmask,
                       OSTime timeout);

BOOL
MPPrintTaskQStats(MPTaskQueue *queue,
                  uint32_t unk);

void
MPInitTask(MPTask *task,
           MPTaskFunc func,
           uint32_t userArg1,
           uint32_t userArg2);

BOOL
MPTermTask(MPTask* task);

BOOL
MPGetTaskInfo(MPTask *task,
              MPTaskInfo *info);

void *
MPGetTaskUserData(MPTask *task);

void
MPSetTaskUserData(MPTask *task,
                  void *userData);

BOOL
MPRunTasksFromTaskQ(MPTaskQueue *queue,
                    uint32_t count);

BOOL
MPRunTask(MPTask *task);

/** @} */

} // namespace coreinit

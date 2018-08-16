#pragma once
#include "coreinit_enum.h"
#include "coreinit_spinlock.h"
#include "coreinit_time.h"
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_taskqueue Task Queue
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

struct MPTaskInfo;
struct MPTask;
struct MPTaskQueueInfo;
struct MPTaskQueue;

using MPTaskFunc = virt_func_ptr<uint32_t(uint32_t arg1,
                                          uint32_t arg2)>;

struct MPTaskInfo
{
   be2_val<MPTaskState> state;
   be2_val<uint32_t> result;
   be2_val<uint32_t> coreID;
   be2_val<OSTime> duration;
};
CHECK_OFFSET(MPTaskInfo, 0x00, state);
CHECK_OFFSET(MPTaskInfo, 0x04, result);
CHECK_OFFSET(MPTaskInfo, 0x08, coreID);
CHECK_OFFSET(MPTaskInfo, 0x0C, duration);
CHECK_SIZE(MPTaskInfo, 0x14);

struct MPTask
{
   be2_virt_ptr<MPTask> self;
   be2_virt_ptr<MPTaskQueue> queue;
   be2_val<MPTaskState> state;
   be2_val<MPTaskFunc> func;
   be2_val<uint32_t> userArg1;
   be2_val<uint32_t> userArg2;
   be2_val<uint32_t> result;
   be2_val<uint32_t> coreID;
   be2_val<OSTime> duration;
   be2_virt_ptr<void> userData;
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
   be2_val<MPTaskQueueState> state;
   be2_val<uint32_t> tasks;
   be2_val<uint32_t> tasksReady;
   be2_val<uint32_t> tasksRunning;
   be2_val<uint32_t> tasksFinished;
};
CHECK_OFFSET(MPTaskQueueInfo, 0x00, state);
CHECK_OFFSET(MPTaskQueueInfo, 0x04, tasks);
CHECK_OFFSET(MPTaskQueueInfo, 0x08, tasksReady);
CHECK_OFFSET(MPTaskQueueInfo, 0x0C, tasksRunning);
CHECK_OFFSET(MPTaskQueueInfo, 0x10, tasksFinished);
CHECK_SIZE(MPTaskQueueInfo, 0x14);

struct MPTaskQueue
{
   be2_virt_ptr<MPTaskQueue> self;
   be2_val<MPTaskQueueState> state;
   be2_val<uint32_t> tasks;
   be2_val<uint32_t> tasksReady;
   be2_val<uint32_t> tasksRunning;
   UNKNOWN(4);
   be2_val<uint32_t> tasksFinished;
   UNKNOWN(8);
   be2_val<uint32_t> queueIndex;
   UNKNOWN(8);
   be2_val<uint32_t> queueSize;
   UNKNOWN(4);
   be2_virt_ptr<virt_ptr<MPTask>> queue;
   be2_val<uint32_t> queueMaxSize;
   be2_struct<OSSpinLock> lock;
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
MPInitTaskQ(virt_ptr<MPTaskQueue> queue,
            virt_ptr<virt_ptr<MPTask>> taskBuffer,
            uint32_t taskBufferLen);

BOOL
MPTermTaskQ(virt_ptr<MPTaskQueue> queue);

BOOL
MPGetTaskQInfo(virt_ptr<MPTaskQueue> queue,
               virt_ptr<MPTaskQueueInfo> info);

BOOL
MPStartTaskQ(virt_ptr<MPTaskQueue> queue);

BOOL
MPStopTaskQ(virt_ptr<MPTaskQueue> queue);

BOOL
MPResetTaskQ(virt_ptr<MPTaskQueue> queue);

BOOL
MPEnqueTask(virt_ptr<MPTaskQueue> queue,
            virt_ptr<MPTask> task);

virt_ptr<MPTask>
MPDequeTask(virt_ptr<MPTaskQueue> queue);

uint32_t
MPDequeTasks(virt_ptr<MPTaskQueue> queue,
             virt_ptr<virt_ptr<MPTask>> taskBuffer,
             uint32_t taskBufferLen);

BOOL
MPWaitTaskQ(virt_ptr<MPTaskQueue> queue,
            MPTaskQueueState mask);

BOOL
MPWaitTaskQWithTimeout(virt_ptr<MPTaskQueue> queue,
                       MPTaskQueueState wmask,
                       OSTimeNanoseconds timeoutNS);

BOOL
MPPrintTaskQStats(virt_ptr<MPTaskQueue> queue,
                  uint32_t unk);

void
MPInitTask(virt_ptr<MPTask> task,
           MPTaskFunc func,
           uint32_t userArg1,
           uint32_t userArg2);

BOOL
MPTermTask(virt_ptr<MPTask> task);

BOOL
MPGetTaskInfo(virt_ptr<MPTask> task,
              virt_ptr<MPTaskInfo> info);

virt_ptr<void>
MPGetTaskUserData(virt_ptr<MPTask> task);

void
MPSetTaskUserData(virt_ptr<MPTask> task,
                  virt_ptr<void> userData);

BOOL
MPRunTasksFromTaskQ(virt_ptr<MPTaskQueue> queue,
                    uint32_t count);

BOOL
MPRunTask(virt_ptr<MPTask> task);

/** @} */

} // namespace cafe::coreinit

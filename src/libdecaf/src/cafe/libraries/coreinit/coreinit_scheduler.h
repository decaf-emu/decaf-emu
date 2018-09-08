#pragma once
#include "coreinit_time.h"
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

struct OSThread;
struct OSThreadQueue;
struct OSThreadSimpleQueue;

namespace internal
{

virt_ptr<OSThread>
getCoreRunningThread(uint32_t coreId);

uint64_t
getCoreThreadRunningTime(uint32_t coreId);

void
pauseCoreTime(bool isPaused);

virt_ptr<OSThread>
getFirstActiveThread();

virt_ptr<OSThread>
getCurrentThread();

void
lockScheduler();

bool
isSchedulerLocked();

void
unlockScheduler();

bool
isSchedulerEnabled();

void
enableScheduler();

void
disableScheduler();

void
markThreadActiveNoLock(virt_ptr<OSThread> thread);

void
markThreadInactiveNoLock(virt_ptr<OSThread> thread);

bool
isThreadActiveNoLock(virt_ptr<OSThread> thread);

void
setThreadAffinityNoLock(virt_ptr<OSThread> thread,
                        uint32_t affinity);

int32_t
checkActiveThreadsNoLock();

void
checkRunningThreadNoLock(bool yielding);

void
rescheduleNoLock(uint32_t core);

void
rescheduleSelfNoLock();

void
rescheduleOtherCoreNoLock();

void
rescheduleAllCoreNoLock();

int32_t
resumeThreadNoLock(virt_ptr<OSThread> thread,
                   int32_t counter);

void
setCoreRunningThread(uint32_t coreId,
                     virt_ptr<OSThread> thread);

bool
setThreadRunQuantumNoLock(virt_ptr<OSThread> thread,
                          OSTime ticks);

void
sleepThreadNoLock(virt_ptr<OSThreadQueue> queue);

void
sleepThreadNoLock(virt_ptr<OSThreadSimpleQueue> queue);

void
suspendThreadNoLock(virt_ptr<OSThread> thread);

void
testThreadCancelNoLock();

void
wakeupOneThreadNoLock(virt_ptr<OSThread> thread);

void
wakeupThreadNoLock(virt_ptr<OSThreadQueue> queue);

void
wakeupThreadNoLock(virt_ptr<OSThreadSimpleQueue> queue);

void
wakeupThreadWaitForSuspensionNoLock(virt_ptr<OSThreadQueue> queue,
                                    int32_t suspendResult);

int32_t
calculateThreadPriorityNoLock(virt_ptr<OSThread> thread);

virt_ptr<OSThread>
setThreadActualPriorityNoLock(virt_ptr<OSThread> thread,
                              int32_t priority);

void
updateThreadPriorityNoLock(virt_ptr<OSThread> thread);

void
promoteThreadPriorityNoLock(virt_ptr<OSThread> thread,
                            int32_t priority);

void
initialiseScheduler();

} // namespace internal

} // namespace cafe::coreinit

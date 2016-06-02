#pragma once
#include "types.h"
#include "coreinit_thread.h"

namespace coreinit
{

struct OSThread;
struct OSThreadQueue;

namespace internal
{

OSThread *
getCurrentThread();

void
lockScheduler();

bool
isSchedulerLocked();

void
unlockScheduler();

void
enableScheduler();

void
disableScheduler();

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
resumeThreadNoLock(OSThread *thread, int32_t counter);

void
sleepThreadNoLock(OSThreadQueue *queue);

void
suspendThreadNoLock(OSThread *thread);

void
testThreadCancelNoLock();

void
unqueueThreadNoLock(OSThread *thread);

void
wakeupOneThreadNoLock(OSThread *thread);

void
wakeupThreadNoLock(OSThreadQueue *queue);

void
wakeupThreadWaitForSuspensionNoLock(OSThreadQueue *queue, int32_t suspendResult);

} // namespace internal

} // namespace coreinit



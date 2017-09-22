#pragma once
#include "ios/ios_enum.h"
#include <libcpu/be2_struct.h>

namespace ios::kernel
{

struct Thread;
struct ThreadQueue;

namespace internal
{

void
lockScheduler();

void
unlockScheduler();

void
sleepThreadNoLock(phys_ptr<ThreadQueue> queue);

void
wakeupOneThreadNoLock(phys_ptr<ThreadQueue> queue,
                      Error waitResult);

void
wakeupAllThreadsNoLock(phys_ptr<ThreadQueue> queue,
                       Error waitResult);

void
queueThreadNoLock(phys_ptr<Thread> thread);

bool
isThreadInRunQueue(phys_ptr<Thread> thread);

void
rescheduleAllNoLock();

void
rescheduleSelfNoLock();

void
handleSchedulerInterrupt();

} // namespace internal

} // namespace ios::kernel

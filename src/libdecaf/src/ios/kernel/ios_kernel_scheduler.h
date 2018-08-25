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
sleepThread(phys_ptr<ThreadQueue> queue);

void
wakeupOneThread(phys_ptr<ThreadQueue> queue,
                Error waitResult);

void
wakeupAllThreads(phys_ptr<ThreadQueue> queue,
                 Error waitResult);

void
queueThread(phys_ptr<Thread> thread);

bool
isThreadInRunQueue(phys_ptr<Thread> thread);

void
reschedule(bool yielding = false);

void
setIdleFiber();

void
initialiseStaticSchedulerData();

} // namespace internal

} // namespace ios::kernel

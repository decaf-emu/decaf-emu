#include "coreinit.h"
#include "coreinit_scheduler.h"

static std::atomic_bool
gSchedulerLock = false;

void
OSLockScheduler()
{
   bool locked = false;

   while (!gSchedulerLock.compare_exchange_weak(locked, true, std::memory_order_acquire)) {
      locked = false;
   }
}

void
OSUnlockScheduler()
{
   gSchedulerLock.store(false, std::memory_order_release);
}

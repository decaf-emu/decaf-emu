#include "coreinit_internal_idlock.h"
#include "libcpu/mem.h"

namespace coreinit
{

namespace internal
{

void
acquireIdLock(IdLock &lock, uint32_t id)
{
   uint32_t expected = 0;

   while (!lock.owner.compare_exchange_weak(expected, id, std::memory_order_acquire)) {
      expected = 0;
   }
}

void
releaseIdLock(IdLock &lock, uint32_t id)
{
   lock.owner.store(0, std::memory_order_release);
}

void
acquireIdLock(IdLock &lock, void *owner)
{
   acquireIdLock(lock, mem::untranslate(owner));
}

void
releaseIdLock(IdLock &lock, void *owner)
{
   releaseIdLock(lock, mem::untranslate(owner));
}

void
acquireIdLock(IdLock &lock)
{
   acquireIdLock(lock, 0xDEADBEEF);
}

void
releaseIdLock(IdLock &lock)
{
   releaseIdLock(lock, 0xDEADBEEF);
}

} // namespace internal

} // namespace coreinit

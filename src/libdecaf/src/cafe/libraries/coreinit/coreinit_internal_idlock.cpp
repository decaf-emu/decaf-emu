#include "coreinit_internal_idlock.h"
#include <libcpu/cpu.h>

namespace cafe::coreinit::internal
{

static uint32_t
getCoreLockId()
{
   auto id = cpu::this_core::id();
   auto core = 1u << id;

   if (id == cpu::InvalidCoreId) {
      core = 1u << 31;
   }

   return core;
}

bool
acquireIdLock(IdLock &lock,
              uint32_t id)
{
   auto expected = 0u;
   if (id == 0) {
      return false;
   }

   while (!lock.owner.compare_exchange_weak(expected, id, std::memory_order_acquire)) {
      expected = 0;
   }

   return true;
}

bool
acquireIdLock(IdLock &lock,
              virt_ptr<void> owner)
{
   return acquireIdLock(lock,
                        static_cast<uint32_t>(virt_cast<virt_addr>(owner)));
}

bool
acquireIdLockWithCoreId(IdLock &lock)
{
   return acquireIdLock(lock, getCoreLockId());
}

bool
releaseIdLock(IdLock &lock,
              uint32_t id)
{
   auto owner = lock.owner.exchange(0, std::memory_order_release);
   return (owner == id);
}

bool
releaseIdLock(IdLock &lock,
              virt_ptr<void> owner)
{
   return releaseIdLock(lock,
                        static_cast<uint32_t>(virt_cast<virt_addr>(owner)));
}

bool
releaseIdLockWithCoreId(IdLock &lock)
{
   return releaseIdLock(lock, getCoreLockId());
}

bool
isHoldingIdLock(IdLock &lock,
                uint32_t id)
{
   return lock.owner.load(std::memory_order_acquire) == id;
}

bool
isHoldingIdLock(IdLock &lock,
                virt_ptr<void> owner)
{
   return isHoldingIdLock(lock,
                          static_cast<uint32_t>(virt_cast<virt_addr>(owner)));
}

bool
isHoldingIdLockWithCoreId(IdLock &lock)
{
   return isHoldingIdLock(lock, getCoreLockId());
}

bool
isLockHeldBySomeone(IdLock &lock)
{
   return lock.owner.load(std::memory_order_acquire) != 0;
}

} // namespace namespace cafe::coreinit::internal

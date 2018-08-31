#pragma once
#include <atomic>
#include <libcpu/be2_struct.h>

namespace cafe::coreinit::internal
{

struct IdLock
{
   std::atomic<uint32_t> owner;
};

bool
acquireIdLock(IdLock &lock,
              uint32_t id);

bool
acquireIdLock(IdLock &lock,
              virt_ptr<void> owner);

bool
acquireIdLockWithCoreId(IdLock &lock);

bool
releaseIdLock(IdLock &lock,
              uint32_t id);

bool
releaseIdLock(IdLock &lock,
              virt_ptr<void> owner);

bool
releaseIdLockWithCoreId(IdLock &lock);

bool
isHoldingIdLock(IdLock &lock,
                uint32_t id);

bool
isHoldingIdLock(IdLock &lock,
                virt_ptr<void> owner);

bool
isHoldingIdLockWithCoreId(IdLock &lock);

bool
isLockHeldBySomeone(IdLock &lock);

} // namespace namespace cafe::coreinit::internal

#pragma once
#include <atomic>
#include <cstdint>

namespace cafe::kernel::internal
{

struct SpinLock
{
   std::atomic<uint32_t> value;
};

bool
spinLockAcquire(SpinLock &spinLock,
                uint32_t value);

bool
spinLockRelease(SpinLock &spinLock,
                uint32_t expected);

void
kernelLockAcquire();

void
kernelLockRelease();

} // namespace cafe::kernel::internal

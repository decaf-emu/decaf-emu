#include "cafe_kernel_lock.h"
#include <atomic>
#include <libcpu/cpu.h>

namespace cafe::kernel::internal
{

static SpinLock
sKernelLock { 0 };

bool
spinLockAcquire(SpinLock &spinLock,
                uint32_t value)
{
   auto expected = 0u;

   if (!value) {
      return false;
   }

   while (!spinLock.value.compare_exchange_weak(expected, value, std::memory_order_acquire)) {
      expected = 0;
   }

   return true;
}

bool
spinLockRelease(SpinLock &spinLock,
                uint32_t expected)
{
   auto value = spinLock.value.exchange(0, std::memory_order_release);
   return (value == expected);
}


void
kernelLockAcquire()
{
   spinLockAcquire(sKernelLock, cpu::this_core::id() + 1);
}

void
kernelLockRelease()
{
   spinLockRelease(sKernelLock, cpu::this_core::id() + 1);
}

} // namespace cafe::kernel::internal

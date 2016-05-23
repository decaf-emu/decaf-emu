#include "coreinit.h"
#include "coreinit_atomic64.h"
#include "coreinit_memheap.h"
#include "coreinit_spinlock.h"
#include "utils/bitutils.h"
#include "types.h"

namespace coreinit
{

static
OSSpinLock *sAtomic64Lock;

uint64_t
OSGetAtomic64(be_val<uint64_t> *ptr)
{
   ScopedSpinLock lock(sAtomic64Lock);
   return *ptr;
}

uint64_t
OSSetAtomic64(be_val<uint64_t> *ptr, uint64_t value)
{
   ScopedSpinLock lock(sAtomic64Lock);
   auto old = *ptr;
   *ptr = value;
   return old;
}

BOOL
OSCompareAndSwapAtomic64(be_val<uint64_t> *ptr, uint64_t compare, uint64_t value)
{
   ScopedSpinLock lock(sAtomic64Lock);

   if (*ptr == compare) {
      *ptr = value;
      return TRUE;
   } else {
      return FALSE;
   }
}

BOOL
OSCompareAndSwapAtomicEx64(be_val<uint64_t> *ptr, uint64_t compare, uint64_t value, uint64_t *old)
{
   ScopedSpinLock lock(sAtomic64Lock);
   *old = *ptr;

   if (*ptr == compare) {
      *ptr = value;
      return TRUE;
   } else {
      return FALSE;
   }
}

uint64_t
OSSwapAtomic64(be_val<uint64_t> *ptr, uint64_t value)
{
   ScopedSpinLock lock(sAtomic64Lock);
   auto old = *ptr;
   *ptr = value;
   return old;
}

int64_t
OSAddAtomic64(be_val<int64_t> *ptr, int64_t value)
{
   ScopedSpinLock lock(sAtomic64Lock);
   return (*ptr += value);
}

uint64_t
OSAndAtomic64(be_val<uint64_t> *ptr, uint64_t value)
{
   ScopedSpinLock lock(sAtomic64Lock);
   return (*ptr &= value);
}

uint64_t
OSOrAtomic64(be_val<uint64_t> *ptr, uint64_t value)
{
   ScopedSpinLock lock(sAtomic64Lock);
   return (*ptr |= value);
}

uint64_t
OSXorAtomic64(be_val<uint64_t> *ptr, uint64_t value)
{
   ScopedSpinLock lock(sAtomic64Lock);
   return (*ptr ^= value);
}

BOOL
OSTestAndClearAtomic64(be_val<uint64_t> *ptr, uint32_t bit)
{
   ScopedSpinLock lock(sAtomic64Lock);
   auto result = get_bit(*ptr, bit) ? TRUE : FALSE;
   *ptr = clear_bit(*ptr, bit);
   return result;
}

BOOL
OSTestAndSetAtomic64(be_val<uint64_t> *ptr, uint32_t bit)
{
   ScopedSpinLock lock(sAtomic64Lock);
   auto result = get_bit(*ptr, bit) ? TRUE : FALSE;
   *ptr = set_bit(*ptr, bit);
   return result;
}

void
Module::initialiseAtomic64()
{
   sAtomic64Lock = coreinit::internal::sysAlloc<OSSpinLock>();
}

void
Module::registerAtomic64Functions()
{
   RegisterKernelFunction(OSGetAtomic64);
   RegisterKernelFunction(OSSetAtomic64);
   RegisterKernelFunction(OSCompareAndSwapAtomic64);
   RegisterKernelFunction(OSCompareAndSwapAtomicEx64);
   RegisterKernelFunction(OSSwapAtomic64);
   RegisterKernelFunction(OSAddAtomic64);
   RegisterKernelFunction(OSAndAtomic64);
   RegisterKernelFunction(OSOrAtomic64);
   RegisterKernelFunction(OSXorAtomic64);
   RegisterKernelFunction(OSTestAndClearAtomic64);
   RegisterKernelFunction(OSTestAndSetAtomic64);
}

} // namespace coreinit

#include "coreinit.h"
#include "coreinit_atomic64.h"
#include "coreinit_memheap.h"
#include "coreinit_internal_idlock.h"

#include <common/bitutils.h>

namespace coreinit
{

static
internal::IdLock sAtomic64Lock;

uint64_t
OSGetAtomic64(be_val<uint64_t> *ptr)
{
   internal::acquireIdLock(sAtomic64Lock);
   auto value = *ptr;
   internal::releaseIdLock(sAtomic64Lock);
   return value;
}

uint64_t
OSSetAtomic64(be_val<uint64_t> *ptr,
              uint64_t value)
{
   internal::acquireIdLock(sAtomic64Lock);
   auto old = *ptr;
   *ptr = value;
   internal::releaseIdLock(sAtomic64Lock);
   return old;
}

BOOL
OSCompareAndSwapAtomic64(be_val<uint64_t> *ptr,
                         uint64_t compare,
                         uint64_t value)
{
   BOOL result = FALSE;
   internal::acquireIdLock(sAtomic64Lock);

   if (*ptr == compare) {
      *ptr = value;
      result = TRUE;
   }

   internal::releaseIdLock(sAtomic64Lock);
   return result;
}

BOOL
OSCompareAndSwapAtomicEx64(be_val<uint64_t> *ptr,
                           uint64_t compare,
                           uint64_t value,
                           be_val<uint64_t> *old)
{
   BOOL result = FALSE;
   internal::acquireIdLock(sAtomic64Lock);
   *old = *ptr;

   if (*ptr == compare) {
      *ptr = value;
      result = TRUE;
   }

   internal::releaseIdLock(sAtomic64Lock);
   return result;
}

uint64_t
OSSwapAtomic64(be_val<uint64_t> *ptr,
               uint64_t value)
{
   internal::acquireIdLock(sAtomic64Lock);
   auto old = *ptr;
   *ptr = value;
   internal::releaseIdLock(sAtomic64Lock);
   return old;
}

int64_t
OSAddAtomic64(be_val<int64_t> *ptr,
              int64_t value)
{
   internal::acquireIdLock(sAtomic64Lock);
   auto result = (*ptr += value);
   internal::releaseIdLock(sAtomic64Lock);
   return result;
}

uint64_t
OSAndAtomic64(be_val<uint64_t> *ptr,
              uint64_t value)
{
   internal::acquireIdLock(sAtomic64Lock);
   auto result = (*ptr &= value);
   internal::releaseIdLock(sAtomic64Lock);
   return result;
}

uint64_t
OSOrAtomic64(be_val<uint64_t> *ptr,
             uint64_t value)
{
   internal::acquireIdLock(sAtomic64Lock);
   auto result = (*ptr |= value);
   internal::releaseIdLock(sAtomic64Lock);
   return result;
}

uint64_t
OSXorAtomic64(be_val<uint64_t> *ptr,
              uint64_t value)
{
   internal::acquireIdLock(sAtomic64Lock);
   auto result = (*ptr ^= value);
   internal::releaseIdLock(sAtomic64Lock);
   return result;
}

BOOL
OSTestAndClearAtomic64(be_val<uint64_t> *ptr,
                       uint32_t bit)
{
   internal::acquireIdLock(sAtomic64Lock);
   auto result = get_bit(*ptr, bit) ? TRUE : FALSE;
   *ptr = clear_bit(*ptr, bit);
   internal::releaseIdLock(sAtomic64Lock);
   return result;
}

BOOL
OSTestAndSetAtomic64(be_val<uint64_t> *ptr,
                     uint32_t bit)
{
   internal::acquireIdLock(sAtomic64Lock);
   auto result = get_bit(*ptr, bit) ? TRUE : FALSE;
   *ptr = set_bit(*ptr, bit);
   internal::releaseIdLock(sAtomic64Lock);
   return result;
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

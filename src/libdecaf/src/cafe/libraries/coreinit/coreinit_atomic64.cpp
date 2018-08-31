#include "coreinit.h"
#include "coreinit_atomic64.h"
#include "coreinit_internal_idlock.h"

#include <common/bitutils.h>

namespace cafe::coreinit
{

static internal::IdLock sAtomic64Lock;

uint64_t
OSGetAtomic64(virt_ptr<uint64_t> ptr)
{
   internal::acquireIdLockWithCoreId(sAtomic64Lock);
   auto value = *ptr;
   internal::releaseIdLockWithCoreId(sAtomic64Lock);
   return value;
}

uint64_t
OSSetAtomic64(virt_ptr<uint64_t> ptr,
              uint64_t value)
{
   internal::acquireIdLockWithCoreId(sAtomic64Lock);
   auto old = *ptr;
   *ptr = value;
   internal::releaseIdLockWithCoreId(sAtomic64Lock);
   return old;
}

BOOL
OSCompareAndSwapAtomic64(virt_ptr<uint64_t> ptr,
                         uint64_t compare,
                         uint64_t value)
{
   BOOL result = FALSE;
   internal::acquireIdLockWithCoreId(sAtomic64Lock);

   if (*ptr == compare) {
      *ptr = value;
      result = TRUE;
   }

   internal::releaseIdLockWithCoreId(sAtomic64Lock);
   return result;
}

BOOL
OSCompareAndSwapAtomicEx64(virt_ptr<uint64_t> ptr,
                           uint64_t compare,
                           uint64_t value,
                           virt_ptr<uint64_t> old)
{
   BOOL result = FALSE;
   internal::acquireIdLockWithCoreId(sAtomic64Lock);
   *old = *ptr;

   if (*ptr == compare) {
      *ptr = value;
      result = TRUE;
   }

   internal::releaseIdLockWithCoreId(sAtomic64Lock);
   return result;
}

uint64_t
OSSwapAtomic64(virt_ptr<uint64_t> ptr,
               uint64_t value)
{
   internal::acquireIdLockWithCoreId(sAtomic64Lock);
   auto old = *ptr;
   *ptr = value;
   internal::releaseIdLockWithCoreId(sAtomic64Lock);
   return old;
}

int64_t
OSAddAtomic64(virt_ptr<int64_t> ptr,
              int64_t value)
{
   internal::acquireIdLockWithCoreId(sAtomic64Lock);
   auto result = (*ptr += value);
   internal::releaseIdLockWithCoreId(sAtomic64Lock);
   return result;
}

uint64_t
OSAndAtomic64(virt_ptr<uint64_t> ptr,
              uint64_t value)
{
   internal::acquireIdLockWithCoreId(sAtomic64Lock);
   auto result = (*ptr &= value);
   internal::releaseIdLockWithCoreId(sAtomic64Lock);
   return result;
}

uint64_t
OSOrAtomic64(virt_ptr<uint64_t> ptr,
             uint64_t value)
{
   internal::acquireIdLockWithCoreId(sAtomic64Lock);
   auto result = (*ptr |= value);
   internal::releaseIdLockWithCoreId(sAtomic64Lock);
   return result;
}

uint64_t
OSXorAtomic64(virt_ptr<uint64_t> ptr,
              uint64_t value)
{
   internal::acquireIdLockWithCoreId(sAtomic64Lock);
   auto result = (*ptr ^= value);
   internal::releaseIdLockWithCoreId(sAtomic64Lock);
   return result;
}

BOOL
OSTestAndClearAtomic64(virt_ptr<uint64_t> ptr,
                       uint32_t bit)
{
   internal::acquireIdLockWithCoreId(sAtomic64Lock);
   auto result = get_bit<uint64_t>(*ptr, bit) ? TRUE : FALSE;
   *ptr = clear_bit<uint64_t>(*ptr, bit);
   internal::releaseIdLockWithCoreId(sAtomic64Lock);
   return result;
}

BOOL
OSTestAndSetAtomic64(virt_ptr<uint64_t> ptr,
                     uint32_t bit)
{
   internal::acquireIdLockWithCoreId(sAtomic64Lock);
   auto result = get_bit<uint64_t>(*ptr, bit) ? TRUE : FALSE;
   *ptr = set_bit<uint64_t>(*ptr, bit);
   internal::releaseIdLockWithCoreId(sAtomic64Lock);
   return result;
}

void
Library::registerAtomic64Symbols()
{
   RegisterFunctionExport(OSGetAtomic64);
   RegisterFunctionExport(OSSetAtomic64);
   RegisterFunctionExport(OSCompareAndSwapAtomic64);
   RegisterFunctionExport(OSCompareAndSwapAtomicEx64);
   RegisterFunctionExport(OSSwapAtomic64);
   RegisterFunctionExport(OSAddAtomic64);
   RegisterFunctionExport(OSAndAtomic64);
   RegisterFunctionExport(OSOrAtomic64);
   RegisterFunctionExport(OSXorAtomic64);
   RegisterFunctionExport(OSTestAndClearAtomic64);
   RegisterFunctionExport(OSTestAndSetAtomic64);
}

} // namespace cafe::coreinit

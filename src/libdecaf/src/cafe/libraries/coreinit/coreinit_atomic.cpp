#include "coreinit.h"
#include "coreinit_atomic.h"

#include <atomic>
#include <common/byte_swap.h>

namespace cafe::coreinit
{

BOOL
OSCompareAndSwapAtomic(virt_ptr<be2_atomic<uint32_t>> atomic,
                       uint32_t compare,
                       uint32_t value)
{
   return atomic->compare_exchange_strong(compare, value) ? TRUE : FALSE;
}

BOOL
OSCompareAndSwapAtomicEx(virt_ptr<be2_atomic<uint32_t>> atomic,
                         uint32_t compare,
                         uint32_t value,
                         virt_ptr<uint32_t> old)
{
   auto result = atomic->compare_exchange_strong(compare, value);
   *old = compare;
   return result ? TRUE : FALSE;
}

uint32_t
OSSwapAtomic(virt_ptr<be2_atomic<uint32_t>> atomic,
             uint32_t value)
{
   return atomic->exchange(value);
}

int32_t
OSAddAtomic(virt_ptr<be2_atomic<int32_t>> atomic,
            int32_t value)
{
   return atomic->fetch_add(value);
}

uint32_t
OSAndAtomic(virt_ptr<be2_atomic<uint32_t>> atomic,
            uint32_t value)
{
   return atomic->fetch_and(value);
}

uint32_t
OSOrAtomic(virt_ptr<be2_atomic<uint32_t>> atomic,
           uint32_t value)
{
   return atomic->fetch_or(value);
}

uint32_t
OSXorAtomic(virt_ptr<be2_atomic<uint32_t>> atomic,
            uint32_t value)
{
   return atomic->fetch_xor(value);
}

BOOL
OSTestAndClearAtomic(virt_ptr<be2_atomic<uint32_t>> atomic,
                     uint32_t bit)
{
   auto previous = OSAndAtomic(atomic, ~(1 << bit));

   if (previous & (1 << bit)) {
      return TRUE;
   } else {
      return FALSE;
   }
}

BOOL
OSTestAndSetAtomic(virt_ptr<be2_atomic<uint32_t>> atomic,
                   uint32_t bit)
{
   auto previous = OSOrAtomic(atomic, 1 << bit);

   if (previous & (1 << bit)) {
      return TRUE;
   } else {
      return FALSE;
   }
}

void
Library::registerAtomicSymbols()
{
   RegisterFunctionExport(OSCompareAndSwapAtomic);
   RegisterFunctionExport(OSCompareAndSwapAtomicEx);
   RegisterFunctionExport(OSSwapAtomic);
   RegisterFunctionExport(OSAddAtomic);
   RegisterFunctionExport(OSAndAtomic);
   RegisterFunctionExport(OSOrAtomic);
   RegisterFunctionExport(OSXorAtomic);
   RegisterFunctionExport(OSTestAndClearAtomic);
   RegisterFunctionExport(OSTestAndSetAtomic);
}

} // namespace coreinit

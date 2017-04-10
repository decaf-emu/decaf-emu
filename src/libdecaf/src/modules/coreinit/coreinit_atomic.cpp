#include "coreinit.h"
#include "coreinit_atomic.h"

#include <atomic>
#include <common/byte_swap.h>

namespace coreinit
{

BOOL
OSCompareAndSwapAtomic(be_val<uint32_t> *ptr,
                       uint32_t compare,
                       uint32_t value)
{
   compare = byte_swap(compare);
   value = byte_swap(value);
   return std::atomic_compare_exchange_strong(reinterpret_cast<volatile std::atomic<uint32_t> *>(ptr),
                                              &compare,
                                              value) ? TRUE : FALSE;
}

BOOL
OSCompareAndSwapAtomicEx(be_val<uint32_t> *ptr,
                         uint32_t compare,
                         uint32_t value,
                         be_val<uint32_t> *old)
{
   compare = byte_swap(compare);
   value = byte_swap(value);

   auto success = std::atomic_compare_exchange_strong(reinterpret_cast<volatile std::atomic<uint32_t> *>(ptr),
                                                      &compare,
                                                      value);

   *old = byte_swap(compare);
   return success ? TRUE : FALSE;
}

uint32_t
OSSwapAtomic(be_val<uint32_t> *ptr,
             uint32_t value)
{
   value = byte_swap(value);
   value = std::atomic_exchange(reinterpret_cast<volatile std::atomic<uint32_t> *>(ptr),
                                value);
   return byte_swap(value);
}

int32_t
OSAddAtomic(be_val<int32_t> *ptr,
            int32_t value)
{
   int32_t newValue;
   int32_t compare;

   do {
      compare = std::atomic_load(reinterpret_cast<volatile std::atomic<int32_t> *>(ptr));
      newValue = byte_swap(byte_swap(compare) + value);
   } while (!OSCompareAndSwapAtomic(reinterpret_cast<be_val<uint32_t> *>(ptr), compare, newValue));

   return byte_swap(compare);
}

uint32_t
OSAndAtomic(be_val<uint32_t> *ptr,
            uint32_t value)
{
   uint32_t newValue;
   uint32_t compare;

   do {
      compare = std::atomic_load(reinterpret_cast<volatile std::atomic<uint32_t> *>(ptr));
      newValue = byte_swap(byte_swap(compare) & value);
   } while (!OSCompareAndSwapAtomic(ptr, compare, newValue));

   return byte_swap(compare);
}

uint32_t
OSOrAtomic(be_val<uint32_t> *ptr,
           uint32_t value)
{
   uint32_t newValue;
   uint32_t compare;

   do {
      compare = std::atomic_load(reinterpret_cast<volatile std::atomic<uint32_t> *>(ptr));
      newValue = byte_swap(byte_swap(compare) | value);
   } while (!OSCompareAndSwapAtomic(ptr, compare, newValue));

   return byte_swap(compare);
}

uint32_t
OSXorAtomic(be_val<uint32_t> *ptr,
            uint32_t value)
{
   uint32_t newValue;
   uint32_t compare;

   do {
      compare = std::atomic_load(reinterpret_cast<volatile std::atomic<uint32_t> *>(ptr));
      newValue = byte_swap(byte_swap(compare) ^ value);
   } while (!OSCompareAndSwapAtomic(ptr, compare, newValue));

   return byte_swap(compare);
}

BOOL
OSTestAndClearAtomic(be_val<uint32_t> *ptr,
                     uint32_t bit)
{
   auto previous = OSAndAtomic(ptr, ~(1 << bit));

   if (previous & (1 << bit)) {
      return TRUE;
   } else {
      return FALSE;
   }
}

BOOL
OSTestAndSetAtomic(be_val<uint32_t> *ptr,
                   uint32_t bit)
{
   auto previous = OSOrAtomic(ptr, 1 << bit);

   if (previous & (1 << bit)) {
      return TRUE;
   } else {
      return FALSE;
   }
}

void
Module::registerAtomicFunctions()
{
   RegisterKernelFunction(OSCompareAndSwapAtomic);
   RegisterKernelFunction(OSCompareAndSwapAtomicEx);
   RegisterKernelFunction(OSSwapAtomic);
   RegisterKernelFunction(OSAddAtomic);
   RegisterKernelFunction(OSAndAtomic);
   RegisterKernelFunction(OSOrAtomic);
   RegisterKernelFunction(OSXorAtomic);
   RegisterKernelFunction(OSTestAndClearAtomic);
   RegisterKernelFunction(OSTestAndSetAtomic);
}

} // namespace coreinit

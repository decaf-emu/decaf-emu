#include "coreinit.h"
#include "coreinit_atomic.h"

#include <atomic>
#include <common/byte_swap.h>

namespace cafe::coreinit
{

BOOL
OSCompareAndSwapAtomic(virt_ptr<uint32_t> ptr,
                       uint32_t compare,
                       uint32_t value)
{
   compare = byte_swap(compare);
   value = byte_swap(value);
   return std::atomic_compare_exchange_strong(reinterpret_cast<volatile std::atomic<uint32_t> *>(ptr.getRawPointer()),
                                              &compare,
                                              value) ? TRUE : FALSE;
}

BOOL
OSCompareAndSwapAtomicEx(virt_ptr<uint32_t> ptr,
                         uint32_t compare,
                         uint32_t value,
                         virt_ptr<uint32_t> old)
{
   compare = byte_swap(compare);
   value = byte_swap(value);

   auto success = std::atomic_compare_exchange_strong(reinterpret_cast<volatile std::atomic<uint32_t> *>(ptr.getRawPointer()),
                                                      &compare,
                                                      value);

   *old = byte_swap(compare);
   return success ? TRUE : FALSE;
}

uint32_t
OSSwapAtomic(virt_ptr<uint32_t> ptr,
             uint32_t value)
{
   value = byte_swap(value);
   value = std::atomic_exchange(reinterpret_cast<volatile std::atomic<uint32_t> *>(ptr.getRawPointer()),
                                value);
   return byte_swap(value);
}

int32_t
OSAddAtomic(virt_ptr<int32_t> ptr,
            int32_t value)
{
   int32_t newValue;
   int32_t compare;

   do {
      compare = std::atomic_load(reinterpret_cast<volatile std::atomic<int32_t> *>(ptr.getRawPointer()));
      newValue = byte_swap(byte_swap(compare) + value);
   } while (!OSCompareAndSwapAtomic(virt_cast<uint32_t *>(ptr), compare, newValue));

   return byte_swap(compare);
}

uint32_t
OSAndAtomic(virt_ptr<uint32_t> ptr,
            uint32_t value)
{
   uint32_t newValue;
   uint32_t compare;

   do {
      compare = std::atomic_load(reinterpret_cast<volatile std::atomic<uint32_t> *>(ptr.getRawPointer()));
      newValue = byte_swap(byte_swap(compare) & value);
   } while (!OSCompareAndSwapAtomic(ptr, compare, newValue));

   return byte_swap(compare);
}

uint32_t
OSOrAtomic(virt_ptr<uint32_t> ptr,
           uint32_t value)
{
   uint32_t newValue;
   uint32_t compare;

   do {
      compare = std::atomic_load(reinterpret_cast<volatile std::atomic<uint32_t> *>(ptr.getRawPointer()));
      newValue = byte_swap(byte_swap(compare) | value);
   } while (!OSCompareAndSwapAtomic(ptr, compare, newValue));

   return byte_swap(compare);
}

uint32_t
OSXorAtomic(virt_ptr<uint32_t> ptr,
            uint32_t value)
{
   uint32_t newValue;
   uint32_t compare;

   do {
      compare = std::atomic_load(reinterpret_cast<volatile std::atomic<uint32_t> *>(ptr.getRawPointer()));
      newValue = byte_swap(byte_swap(compare) ^ value);
   } while (!OSCompareAndSwapAtomic(ptr, compare, newValue));

   return byte_swap(compare);
}

BOOL
OSTestAndClearAtomic(virt_ptr<uint32_t> ptr,
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
OSTestAndSetAtomic(virt_ptr<uint32_t> ptr,
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

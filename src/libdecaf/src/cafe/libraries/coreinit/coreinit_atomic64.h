#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_atomic64 Atomic64
 * \ingroup coreinit
 * @{
 */

uint64_t
OSGetAtomic64(virt_ptr<uint64_t> ptr);

uint64_t
OSSetAtomic64(virt_ptr<uint64_t> ptr,
              uint64_t value);

BOOL
OSCompareAndSwapAtomic64(virt_ptr<uint64_t> ptr,
                         uint64_t compare,
                         uint64_t value);

BOOL
OSCompareAndSwapAtomicEx64(virt_ptr<uint64_t> ptr,
                           uint64_t compare,
                           uint64_t value,
                           virt_ptr<uint64_t> old);

uint64_t
OSSwapAtomic64(virt_ptr<uint64_t> ptr,
               uint64_t value);

int64_t
OSAddAtomic64(virt_ptr<int64_t> ptr,
              int64_t value);

uint64_t
OSAndAtomic64(virt_ptr<uint64_t> ptr,
              uint64_t value);

uint64_t
OSOrAtomic64(virt_ptr<uint64_t> ptr,
             uint64_t value);

uint64_t
OSXorAtomic64(virt_ptr<uint64_t> ptr,
              uint64_t value);

BOOL
OSTestAndClearAtomic64(virt_ptr<uint64_t> ptr,
                       uint32_t bit);

BOOL
OSTestAndSetAtomic64(virt_ptr<uint64_t> ptr,
                     uint32_t bit);

/** @} */

} // namespace cafe::coreinit

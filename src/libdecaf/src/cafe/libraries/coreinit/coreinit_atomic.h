#pragma once
#include <libcpu/be2_atomic.h>
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_atomic Atomic
 * \ingroup coreinit
 * @{
 */

BOOL
OSCompareAndSwapAtomic(virt_ptr<be2_atomic<uint32_t>> atomic,
                       uint32_t compare,
                       uint32_t value);

BOOL
OSCompareAndSwapAtomicEx(virt_ptr<be2_atomic<uint32_t>> atomic,
                         uint32_t compare,
                         uint32_t value,
                         virt_ptr<uint32_t> old);

uint32_t
OSSwapAtomic(virt_ptr<be2_atomic<uint32_t>> atomic,
             uint32_t value);

int32_t
OSAddAtomic(virt_ptr<be2_atomic<int32_t>> atomic,
            int32_t value);

uint32_t
OSAndAtomic(virt_ptr<be2_atomic<uint32_t>> atomic,
            uint32_t value);

uint32_t
OSOrAtomic(virt_ptr<be2_atomic<uint32_t>> atomic,
           uint32_t value);

uint32_t
OSXorAtomic(virt_ptr<be2_atomic<uint32_t>> atomic,
            uint32_t value);

BOOL
OSTestAndClearAtomic(virt_ptr<be2_atomic<uint32_t>> atomic,
                     uint32_t bit);

BOOL
OSTestAndSetAtomic(virt_ptr<be2_atomic<uint32_t>> atomic,
                   uint32_t bit);

/** @} */

} // namespace cafe::coreinit

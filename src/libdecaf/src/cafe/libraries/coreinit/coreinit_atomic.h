#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_atomic Atomic
 * \ingroup coreinit
 * @{
 */

BOOL
OSCompareAndSwapAtomic(virt_ptr<uint32_t> ptr,
                       uint32_t compare,
                       uint32_t value);

BOOL
OSCompareAndSwapAtomicEx(virt_ptr<uint32_t> ptr,
                         uint32_t compare,
                         uint32_t value,
                         virt_ptr<uint32_t> old);

uint32_t
OSSwapAtomic(virt_ptr<uint32_t> ptr,
             uint32_t value);

int32_t
OSAddAtomic(virt_ptr<int32_t> ptr,
            int32_t value);

uint32_t
OSAndAtomic(virt_ptr<uint32_t> ptr,
            uint32_t value);

uint32_t
OSOrAtomic(virt_ptr<uint32_t> ptr,
           uint32_t value);

uint32_t
OSXorAtomic(virt_ptr<uint32_t> ptr,
            uint32_t value);

BOOL
OSTestAndClearAtomic(virt_ptr<uint32_t> ptr,
                     uint32_t bit);

BOOL
OSTestAndSetAtomic(virt_ptr<uint32_t> ptr,
                   uint32_t bit);

/** @} */

} // namespace cafe::coreinit

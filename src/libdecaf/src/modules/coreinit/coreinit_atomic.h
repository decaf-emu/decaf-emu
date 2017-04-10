#pragma once
#include <cstdint>
#include <common/be_val.h>
#include <common/cbool.h>

namespace coreinit
{

/**
 * \defgroup coreinit_atomic Atomic
 * \ingroup coreinit
 * @{
 */

BOOL
OSCompareAndSwapAtomic(be_val<uint32_t> *ptr,
                       uint32_t compare,
                       uint32_t value);

BOOL
OSCompareAndSwapAtomicEx(be_val<uint32_t> *ptr,
                         uint32_t compare,
                         uint32_t value,
                         be_val<uint32_t> *old);

uint32_t
OSSwapAtomic(be_val<uint32_t> *ptr,
             uint32_t value);

int32_t
OSAddAtomic(be_val<int32_t> *ptr,
            int32_t value);

uint32_t
OSAndAtomic(be_val<uint32_t> *ptr,
            uint32_t value);

uint32_t
OSOrAtomic(be_val<uint32_t> *ptr,
           uint32_t value);

uint32_t
OSXorAtomic(be_val<uint32_t> *ptr,
            uint32_t value);

BOOL
OSTestAndClearAtomic(be_val<uint32_t> *ptr,
                     uint32_t bit);

BOOL
OSTestAndSetAtomic(be_val<uint32_t> *ptr,
                   uint32_t bit);

/** @} */

} // namespace coreinit

#pragma once
#include "gx2_enum.h"

#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

/**
 * \defgroup gx2r_resource GX2R Resource
 * \ingroup gx2
 * @{
 */

using GX2RAllocFuncPtr = virt_func_ptr<
   virt_ptr<void>(GX2RResourceFlags, uint32_t, uint32_t)
>;

using GX2RFreeFuncPtr = virt_func_ptr<
   void(GX2RResourceFlags, virt_ptr<void>)
>;

void
GX2RSetAllocator(GX2RAllocFuncPtr allocFn,
                 GX2RFreeFuncPtr freeFn);

BOOL
GX2RIsUserMemory(GX2RResourceFlags flags);

namespace internal
{

void
initialiseGx2rAllocator();

GX2RResourceFlags
getOptionFlags(GX2RResourceFlags flags);

virt_ptr<void>
gx2rAlloc(GX2RResourceFlags flags,
          uint32_t size,
          uint32_t align);

void
gx2rFree(GX2RResourceFlags flags,
         virt_ptr<void> buffer);

} // namespace internal

/** @} */

} // namespace cafe::gx2

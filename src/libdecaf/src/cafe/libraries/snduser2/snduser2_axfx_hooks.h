#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::snduser2
{

using AXFXAllocFn = virt_func_ptr<
   virt_ptr<void> (uint32_t size)
>;

using AXFXFreeFn = virt_func_ptr<
   void (virt_ptr<void> ptr)
>;

void
AXFXGetHooks(virt_ptr<AXFXAllocFn> allocFn,
             virt_ptr<AXFXFreeFn> freeFn);

void
AXFXSetHooks(AXFXAllocFn allocFn,
             AXFXFreeFn freeFn);

namespace internal
{

virt_ptr<void>
axfxAlloc(uint32_t size);

void
axfxFree(virt_ptr<void> ptr);

void
initialiseAxfxHooks();

} // namespace internal

} // namespace cafe::snduser2

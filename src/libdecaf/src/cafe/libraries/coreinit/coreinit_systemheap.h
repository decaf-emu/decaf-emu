#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

virt_ptr<void>
OSAllocFromSystem(uint32_t size,
                  int32_t align);

void
OSFreeToSystem(virt_ptr<void> ptr);

namespace internal
{

void
dumpSystemHeap();

void
initialiseSystemHeap(virt_ptr<void> base,
                     uint32_t size);

} // namespace internal

} // namespace cafe::coreinit

#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::ghs
{

virt_ptr<void>
malloc(uint32_t size);

void
free(virt_ptr<void> ptr);

} // namespace cafe::ghs

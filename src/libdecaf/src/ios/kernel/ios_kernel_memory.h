#pragma once
#include <libcpu/be2_struct.h>

namespace ios::kernel
{

phys_ptr<void>
allocMEM0(size_t size);

void
freeMEM0(phys_ptr<void> ptr);

phys_ptr<void>
allocSRAM0(size_t size);

void
freeSRAM0(phys_ptr<void> ptr);

phys_ptr<void>
allocSRAM1(size_t size);

void
freeSRAM1(phys_ptr<void> ptr);

} // namespace ios::kernel

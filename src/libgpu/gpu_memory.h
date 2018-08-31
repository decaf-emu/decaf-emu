#pragma once
#include <libcpu/be2_struct.h>

namespace gpu::internal
{

template<typename Type = void>
Type *
translateAddress(phys_addr address)
{
   return cpu::internal::translate<Type>(address);
}

} // namespace gpu::internal

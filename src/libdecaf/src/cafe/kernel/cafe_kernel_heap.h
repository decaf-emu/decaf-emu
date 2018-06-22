#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::kernel::internal
{

void
initialiseStaticDataHeap();

bool
initialiseWorkAreaHeap();

virt_ptr<void>
allocStaticData(size_t size,
                size_t alignment = 4u);

virt_ptr<void>
allocFromWorkArea(int32_t size,
                  int32_t alignment = 4);

void
freeToWorkArea(virt_ptr<void> ptr);

template<typename Type>
inline virt_ptr<Type>
allocStaticData()
{
   return virt_cast<Type *>(allocStaticData(sizeof(Type), alignof(Type)));
}

} // namespace cafe::kernel::internal

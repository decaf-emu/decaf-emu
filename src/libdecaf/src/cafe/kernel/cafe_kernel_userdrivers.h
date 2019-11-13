#pragma once
#include "cafe_kernel_processid.h"

#include <libcpu/be2_struct.h>

namespace cafe::kernel
{

int32_t
registerUserDriver(virt_ptr<const char> name,
                   uint32_t nameLen,
                   virt_ptr<UniqueProcessId> currentUpid,
                   virt_ptr<UniqueProcessId> ownerUpid);

int32_t
deregisterUserDriver(virt_ptr<const char> name,
                     uint32_t nameLen);

namespace internal
{

void
initialiseStaticUserDriversData();

void
registerRootUserDrivers();

} // namespace internal

} // namespace cafe::kernel

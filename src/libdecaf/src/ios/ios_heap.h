#pragma once
#include "ios_enum.h"

#include <cstdint>
#include <libcpu/be2_struct.h>

namespace ios
{

IOSError
iosHeapInit(phys_addr base,
            uint32_t size);

phys_ptr<void>
iosHeapAlloc(uint32_t size,
             uint32_t alignment);

void
iosHeapFree(phys_ptr<void> ptr);

} // namespace ios

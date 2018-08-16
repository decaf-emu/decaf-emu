#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::loader::internal
{

void
LiSafeFlushCode(virt_addr base,
                uint32_t size);

void
LiFlushDataRangeNoSync(virt_addr base,
                       uint32_t size);

void
Loader_FlushDataRangeNoSync(virt_addr addr,
                            uint32_t size);

} // namespace cafe::loader::internal

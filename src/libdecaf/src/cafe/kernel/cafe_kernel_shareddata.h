#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::kernel
{

enum class SharedAreaId : uint32_t
{
   FontChinese    = 0xFFCAFE01u,
   FontKorean     = 0xFFCAFE02u,
   FontStandard   = 0xFFCAFE03u,
   FontTaiwanese  = 0xFFCAFE04u,
};

struct SharedArea
{
   virt_addr address;
   uint32_t size;
};

void
loadShared();

SharedArea
getSharedArea(SharedAreaId id);

} // namespace cafe::kernel

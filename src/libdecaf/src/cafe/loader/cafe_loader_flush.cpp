#pragma optimize("", off)
#include "cafe_loader_flush.h"
#include "cafe_loader_iop.h"

namespace cafe::loader::internal
{

void
LiSafeFlushCode(virt_addr base, uint32_t size)
{
}

void
LiFlushDataRangeNoSync(virt_addr base, uint32_t size)
{
}

void
Loader_FlushDataRangeNoSync(virt_addr addr, uint32_t size)
{
   LiCheckAndHandleInterrupts();
   while (size > 0) {
      auto flushSize = std::min<uint32_t>(size, 0x20000);
      LiFlushDataRangeNoSync(addr, size);
      LiCheckAndHandleInterrupts();

      addr += flushSize;
      size -= flushSize;
   }
}

} // namespace cafe::loader::internal

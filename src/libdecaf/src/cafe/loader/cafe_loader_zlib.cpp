#pragma optimize("", off)
#include "cafe_loader_iop.h"
#include "cafe_loader_zlib.h"
#include <zlib.h>

namespace cafe::loader::internal
{

uint32_t
LiCalcCRC32(uint32_t crc,
            const virt_ptr<void> data,
            uint32_t size)
{
   LiCheckAndHandleInterrupts();
   if (!data || !size) {
      return crc;
   }

   crc = crc32(crc, reinterpret_cast<Bytef *>(data.getRawPointer()), size);
   LiCheckAndHandleInterrupts();
   return crc;
}

} // namespace cafe::loader::internal

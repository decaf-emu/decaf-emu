#include "cafe_loader_iop.h"
#include "cafe_loader_zlib.h"
#include <zlib.h>

namespace cafe::loader::internal
{

uint32_t
LiCalcCRC32(uint32_t crc,
            virt_ptr<const void> data,
            uint32_t size)
{
   LiCheckAndHandleInterrupts();
   if (!data || !size) {
      return crc;
   }

   crc = crc32(crc, reinterpret_cast<const Bytef *>(data.get()), size);
   LiCheckAndHandleInterrupts();
   return crc;
}

} // namespace cafe::loader::internal

#include "gx2_mem.h"
#include "gx2_state.h"
#include "gpu/pm4_writer.h"

namespace gx2
{

void
GX2Invalidate(GX2InvalidateMode mode,
              void *buffer,
              uint32_t size)
{
   if (!internal::isInitialised()) {
      return;
   }

   auto memStart = mem::untranslate(buffer);
   auto memEnd = memStart + size;

   pm4::write(pm4::DecafInvalidate {
      mode,
      memStart,
      memEnd
   });
}

} // namespace gx2

#include "decaf_graphics.h"
#include "gx2_mem.h"
#include "gx2_state.h"
#include "gpu/commandqueue.h"
#include "gpu/pm4.h"

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

   decaf::getGraphicsDriver()->invalidateMemory(mode, memStart, memEnd);
}

} // namespace gx2

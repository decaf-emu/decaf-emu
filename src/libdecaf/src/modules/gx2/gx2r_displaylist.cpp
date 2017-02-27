#include "gx2r_buffer.h"
#include "gx2r_displaylist.h"
#include "gx2_displaylist.h"
#include <common/log.h>

namespace gx2
{

void
GX2RBeginDisplayListEx(GX2RBuffer *displayList,
                       uint32_t unused,
                       GX2RResourceFlags flags)
{
   if (!displayList || !displayList->buffer) {
      return;
   }

   auto size = displayList->elemCount * displayList->elemSize;
   GX2BeginDisplayListEx(displayList->buffer, size, TRUE);
}

uint32_t
GX2REndDisplayList(GX2RBuffer *displayList)
{
   auto size = GX2EndDisplayList(displayList->buffer);
   decaf_check(size < (displayList->elemCount * displayList->elemSize));
   return size;
}

void
GX2RCallDisplayList(GX2RBuffer *displayList,
                    uint32_t size)
{
   if (!displayList || !displayList->buffer) {
      return;
   }

   GX2CallDisplayList(displayList->buffer, size);
}

void
GX2RDirectCallDisplayList(GX2RBuffer *displayList,
                          uint32_t size)
{
   if (!displayList || !displayList->buffer) {
      return;
   }

   GX2DirectCallDisplayList(displayList->buffer, size);
}

} // namespace gx2

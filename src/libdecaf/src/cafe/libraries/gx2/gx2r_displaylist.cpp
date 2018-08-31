#include "gx2.h"
#include "gx2_displaylist.h"
#include "gx2r_buffer.h"
#include "gx2r_displaylist.h"
#include <common/decaf_assert.h>

namespace cafe::gx2
{

void
GX2RBeginDisplayListEx(virt_ptr<GX2RBuffer> displayList,
                       uint32_t unused,
                       GX2RResourceFlags flags)
{
   if (!displayList || !displayList->buffer) {
      return;
   }

   GX2BeginDisplayListEx(displayList->buffer,
                         displayList->elemCount * displayList->elemSize,
                         TRUE);
}

uint32_t
GX2REndDisplayList(virt_ptr<GX2RBuffer> displayList)
{
   auto size = GX2EndDisplayList(displayList->buffer);
   decaf_check(size < (displayList->elemCount * displayList->elemSize));
   return size;
}

void
GX2RCallDisplayList(virt_ptr<GX2RBuffer> displayList,
                    uint32_t size)
{
   if (!displayList || !displayList->buffer) {
      return;
   }

   GX2CallDisplayList(displayList->buffer, size);
}

void
GX2RDirectCallDisplayList(virt_ptr<GX2RBuffer> displayList,
                          uint32_t size)
{
   if (!displayList || !displayList->buffer) {
      return;
   }

   GX2DirectCallDisplayList(displayList->buffer, size);
}

void
Library::registerGx2rDisplayListSymbols()
{
   RegisterFunctionExport(GX2RBeginDisplayListEx);
   RegisterFunctionExport(GX2REndDisplayList);
   RegisterFunctionExport(GX2RCallDisplayList);
   RegisterFunctionExport(GX2RDirectCallDisplayList);
}

} // namespace cafe::gx2

#include "gx2_addrlib.h"
#include "gx2_aperture.h"
#include "gx2_surface.h"
#include "cpu/mem.h"
#include "common/teenyheap.h"

namespace gx2
{

static GX2ApertureHandle
gUniqueHandle = 1;

void
GX2AllocateTilingApertureEx(GX2Surface *surface,
                            uint32_t level,
                            uint32_t depth,
                            GX2EndianSwapMode endian,
                            be_val<GX2ApertureHandle> *handle,
                            be_ptr<void> *address)
{
   if (handle) {
      *handle = gUniqueHandle++;
   }

   if (address) {
      *address = surface->image;
   }

   // TODO: Untile to temporary memory
}

void
GX2FreeTilingAperture(GX2ApertureHandle handle)
{
   // TODO: Retile from temporary memory to original memory
}

} // namespace gx2

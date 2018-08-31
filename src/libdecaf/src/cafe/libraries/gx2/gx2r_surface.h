#pragma once
#include "gx2_enum.h"

#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

/**
 * \defgroup gx2r_surface GX2R Surface
 * \ingroup gx2
 * @{
 */

struct GX2Surface;

BOOL
GX2RCreateSurface(virt_ptr<GX2Surface> surface,
                  GX2RResourceFlags flags);

BOOL
GX2RCreateSurfaceUserMemory(virt_ptr<GX2Surface> surface,
                            virt_ptr<uint8_t> image,
                            virt_ptr<uint8_t> mipmap,
                            GX2RResourceFlags flags);

void
GX2RDestroySurfaceEx(virt_ptr<GX2Surface> surface,
                     GX2RResourceFlags flags);

virt_ptr<void>
GX2RLockSurfaceEx(virt_ptr<GX2Surface> surface,
                  int32_t level,
                  GX2RResourceFlags flags);

void
GX2RUnlockSurfaceEx(virt_ptr<GX2Surface> surface,
                    int32_t level,
                    GX2RResourceFlags flags);

BOOL
GX2RIsGX2RSurface(GX2RResourceFlags flags);

void
GX2RInvalidateSurface(virt_ptr<GX2Surface> surface,
                      int32_t level,
                      GX2RResourceFlags flags);

/** @} */

} // namespace cafe::gx2

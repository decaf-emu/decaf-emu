#pragma once
#include "common/types.h"
#include "gx2_enum.h"

namespace gx2
{

struct GX2Surface;

bool
GX2RCreateSurface(GX2Surface *surface,
                  GX2RResourceFlags flags);

bool
GX2RCreateSurfaceUserMemory(GX2Surface *surface,
                            uint8_t *image,
                            uint8_t *mipmap,
                            GX2RResourceFlags flags);

void
GX2RDestroySurfaceEx(GX2Surface *surface,
                     GX2RResourceFlags flags);

void *
GX2RLockSurfaceEx(GX2Surface *surface,
                  int32_t level,
                  GX2RResourceFlags flags);

void
GX2RUnlockSurfaceEx(GX2Surface *surface,
                    int32_t level,
                    GX2RResourceFlags flags);

BOOL
GX2RIsGX2RSurface(GX2RResourceFlags flags);

} // namespace gx2

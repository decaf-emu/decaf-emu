#pragma once
#include "gx2_enum.h"

#include <addrlib/addrinterface.h>
#include <cstdint>
#include <libgpu/latte/latte_enum_sq.h>
#include <vector>

namespace cafe::gx2
{

struct GX2Surface;

namespace internal
{

bool
getSurfaceInfo(GX2Surface *surface,
               uint32_t level,
               ADDR_COMPUTE_SURFACE_INFO_OUTPUT *output);

bool
copySurface(GX2Surface *surfaceSrc,
            uint32_t srcLevel,
            uint32_t srcSlice,
            GX2Surface *surfaceDst,
            uint32_t dstLevel,
            uint32_t dstSlice,
            uint8_t *dstImage = nullptr,
            uint8_t *dstMipmap = nullptr);

uint32_t
getSurfaceSliceSwizzle(GX2TileMode tileMode,
                       uint32_t baseSwizzle,
                       uint32_t slice);

uint32_t
calcSliceSize(GX2Surface *surface,
              ADDR_COMPUTE_SURFACE_INFO_OUTPUT *info);

} // namespace internal

} // namespace cafe::gx2

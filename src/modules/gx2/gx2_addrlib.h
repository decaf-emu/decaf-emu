#pragma once
#include <addrlib/addrinterface.h>
#include <cstdint>
#include <vector>
#include "gx2_enum.h"

namespace gx2
{

struct GX2Surface;

namespace internal
{

bool
initAddrLib();

ADDR_HANDLE
getAddrLibHandle();

bool
getSurfaceInfo(GX2Surface *surface,
               uint32_t level,
               ADDR_COMPUTE_SURFACE_INFO_OUTPUT *output);

bool
copySurface(GX2Surface *surfaceSrc,
            uint32_t srcLevel,
            uint32_t srcDepth,
            GX2Surface *surfaceDst,
            uint32_t dstLevel,
            uint32_t dstDepth,
            uint8_t *dstImage = nullptr,
            uint8_t *dstMipmap = nullptr);

bool
convertTiling(GX2Surface *surface,
              std::vector<uint8_t> &image,
              std::vector<uint8_t> &mipmap);

uint32_t
getSurfaceSliceSwizzle(GX2TileMode tileMode,
                       uint32_t baseSwizzle,
                       uint32_t slice);

uint32_t
calcSliceSize(GX2Surface *surface,
              ADDR_COMPUTE_SURFACE_INFO_OUTPUT *info);

} // namespace internal

} // namespace gx2

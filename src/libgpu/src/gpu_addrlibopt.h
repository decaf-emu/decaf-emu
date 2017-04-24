#pragma once

#include <addrlib/addrinterface.h>

namespace gpu
{

namespace addrlibopt
{

bool
copySurfacePixels(uint8_t *dstBasePtr,
                  uint32_t dstWidth,
                  uint32_t dstHeight,
                  ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &dstAddrInput,
                  uint8_t *srcBasePtr,
                  uint32_t srcWidth,
                  uint32_t srcHeight,
                  ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &srcAddrInput,
                  uint32_t bpp,
                  bool isDepth,
                  uint32_t numSamples);

} // namespace addrlibopt

} // namespace gpu

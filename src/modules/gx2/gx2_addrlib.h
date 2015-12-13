#pragma once
#include <addrlib/addrinterface.h>
#include <cstdint>
#include <vector>

struct GX2Surface;

namespace gx2
{

namespace internal
{

bool
initAddrLib();

ADDR_HANDLE
getAddrLibHandle();

bool
getSurfaceInfo(GX2Surface *surface, uint32_t level, ADDR_COMPUTE_SURFACE_INFO_OUTPUT *output);

bool
copySurface(GX2Surface *surfaceSrc, GX2Surface *surfaceDst, uint32_t level, uint32_t depth, std::vector<uint8_t> &image, std::vector<uint8_t> &mipmap);

bool
convertTiling(GX2Surface *surface, std::vector<uint8_t> &image, std::vector<uint8_t> &mipmap);

} // namespace internal

} // namespace gx2

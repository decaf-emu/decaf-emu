#pragma once
#include "gx2_enum.h"

#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

/**
 * \defgroup gx2_aperture Tiling Aperture
 * \ingroup gx2
 * @{
 */

struct GX2Surface;

using GX2ApertureHandle = uint32_t;

void
GX2AllocateTilingApertureEx(virt_ptr<GX2Surface> surface,
                            uint32_t level,
                            uint32_t depth,
                            GX2EndianSwapMode endian,
                            virt_ptr<GX2ApertureHandle> outHandle,
                            virt_ptr<virt_addr> outAddress);

void
GX2FreeTilingAperture(GX2ApertureHandle handle);

namespace internal
{

bool
translateAperture(virt_addr &address,
                  uint32_t &size);

} // namespace internal

/** @} */

} // namespace cafe::gx2

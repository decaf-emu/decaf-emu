#pragma once
#include "gx2_enum.h"

#include <common/be_ptr.h>
#include <common/be_val.h>
#include <cstdint>

namespace gx2
{

/**
 * \defgroup gx2_aperture Tiling Aperture
 * \ingroup gx2
 * @{
 */

struct GX2Surface;

using GX2ApertureHandle = uint32_t;

void
GX2AllocateTilingApertureEx(GX2Surface *surface,
                            uint32_t level,
                            uint32_t depth,
                            GX2EndianSwapMode endian,
                            be_val<GX2ApertureHandle> *handle,
                            be_ptr<void> *address);

void
GX2FreeTilingAperture(GX2ApertureHandle handle);

namespace internal
{

bool
lookupAperture(uint32_t address,
               uint32_t *apertureBase_ret,  // may be null
               uint32_t *apertureSize_ret,  // may be null
               uint32_t *physBase_ret);     // may be null

} // namespace internal

/** @} */

} // namespace gx2

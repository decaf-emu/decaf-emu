#pragma once
#include "types.h"
#include "gx2_enum.h"
#include "utils/be_val.h"
#include "utils/virtual_ptr.h"

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

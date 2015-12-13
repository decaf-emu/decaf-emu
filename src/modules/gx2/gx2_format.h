#pragma once
#include <utility>
#include "types.h"
#include "gx2_enum.h"
#include "gpu/latte_enum_cb.h"
#include "gpu/latte_enum_db.h"
#include "gpu/latte_enum_sq.h"

size_t
GX2GetAttribFormatBytes(GX2AttribFormat format);

GX2EndianSwapMode
GX2GetAttribFormatSwapMode(GX2AttribFormat format);

latte::SQ_DATA_FORMAT
GX2GetAttribFormatDataFormat(GX2AttribFormat type);

std::pair<size_t, size_t>
GX2GetSurfaceBlockSize(GX2SurfaceFormat format);

uint32_t
GX2GetSurfaceElementBits(GX2SurfaceFormat format);

uint32_t
GX2GetSurfaceElementBytes(GX2SurfaceFormat format);

GX2EndianSwapMode
GX2GetSurfaceSwap(GX2SurfaceFormat format);

GX2SurfaceUse
GX2GetSurfaceUse(GX2SurfaceFormat format);

latte::DB_DEPTH_FORMAT
GX2GetSurfaceDepthFormat(GX2SurfaceFormat format);

latte::CB_FORMAT
GX2GetSurfaceColorFormat(GX2SurfaceFormat format);

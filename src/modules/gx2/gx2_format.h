#pragma once
#include <utility>
#include "types.h"
#include "gx2_enum.h"
#include "gpu/latte_enum_cb.h"
#include "gpu/latte_enum_db.h"
#include "gpu/latte_enum_sq.h"

size_t
GX2GetAttribFormatBytes(GX2AttribFormat::Value format);

GX2EndianSwapMode::Value
GX2GetAttribFormatSwapMode(GX2AttribFormat::Value format);

latte::SQ_DATA_FORMAT
GX2GetAttribFormatDataFormat(GX2AttribFormat::Value type);

std::pair<size_t, size_t>
GX2GetSurfaceBlockSize(GX2SurfaceFormat::Value format);

size_t
GX2GetSurfaceElementBytes(GX2SurfaceFormat::Value format);

GX2EndianSwapMode::Value
GX2GetSurfaceSwap(GX2SurfaceFormat::Value format);

GX2SurfaceUse::Value
GX2GetSurfaceUse(GX2SurfaceFormat::Value format);

latte::DB_DEPTH_FORMAT
GX2GetSurfaceDepthFormat(GX2SurfaceFormat::Value format);

latte::CB_FORMAT
GX2GetSurfaceColorFormat(GX2SurfaceFormat::Value format);

size_t
GX2GetTileThickness(GX2TileMode::Value mode);

std::pair<size_t, size_t>
GX2GetMacroTileSize(GX2TileMode::Value mode);

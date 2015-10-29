#pragma once
#include <utility>
#include "types.h"
#include "gx2_enum.h"

std::pair<size_t, size_t>
GX2GetSurfaceBlockSize(GX2SurfaceFormat::Value format);

size_t
GX2GetSurfaceElementBytes(GX2SurfaceFormat::Value format);

size_t
GX2GetTileThickness(GX2TileMode::Value mode);

std::pair<size_t, size_t>
GX2GetMacroTileSize(GX2TileMode::Value mode);

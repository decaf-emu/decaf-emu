#pragma once
#include "gx2_enum.h"

#include <common/cbool.h>
#include <cstdint>
#include <libgpu/latte/latte_enum_cb.h>
#include <libgpu/latte/latte_enum_db.h>
#include <libgpu/latte/latte_enum_sq.h>
#include <utility>

namespace cafe::gx2
{

uint32_t
GX2GetAttribFormatBits(GX2AttribFormat format);

uint32_t
GX2GetSurfaceFormatBits(GX2SurfaceFormat format);

uint32_t
GX2GetSurfaceFormatBitsPerElement(GX2SurfaceFormat format);

BOOL
GX2SurfaceIsCompressed(GX2SurfaceFormat format);

namespace internal
{

uint32_t
getAttribFormatBytes(GX2AttribFormat format);

latte::SQ_DATA_FORMAT
getAttribFormatDataFormat(GX2AttribFormat type);

GX2EndianSwapMode
getAttribFormatSwapMode(GX2AttribFormat format);

latte::SQ_ENDIAN
getAttribFormatEndian(GX2AttribFormat format);

uint32_t
getSurfaceFormatBytesPerElement(GX2SurfaceFormat format);

GX2SurfaceUse
getSurfaceUse(GX2SurfaceFormat format);

latte::CB_FORMAT
getSurfaceFormatColorFormat(GX2SurfaceFormat format);

latte::CB_NUMBER_TYPE
getSurfaceFormatColorNumberType(GX2SurfaceFormat format);

latte::DB_FORMAT
getSurfaceFormatDepthFormat(GX2SurfaceFormat format);

latte::SQ_ENDIAN
getSurfaceFormatEndian(GX2SurfaceFormat format);

GX2EndianSwapMode
getSurfaceFormatSwapMode(GX2SurfaceFormat format);

GX2SurfaceFormatType
getSurfaceFormatType(GX2SurfaceFormat format);

latte::SQ_ENDIAN
getSwapModeEndian(GX2EndianSwapMode mode);

} // namespace internal

} // namespace cafe::gx2
